#pragma once

#include <memory>

// NOTE: During an active emit connections MUST NOT be removed, nor .clear() be called!

struct Connection;

enum SignalFlags {
  SIGNAL_DEFAULT = 0,
  SIGNAL_ONCE    = 0x01,
  SIGNAL_PREPEND = 0x02
};

namespace detail {

template <typename T>
struct simple_optional {
  simple_optional() = default;

  template <typename U = T>
  simple_optional(U&& u) : active(true) {
    new(&val) T((U&&)u);
  }

  ~simple_optional() {
    if (active) {
      val.~T();
    }
  }

  simple_optional &operator=(const simple_optional &rhs) = delete;

  explicit operator bool() const noexcept {
    return active;
  }

  T &operator*() {
    // assert(active);
    return val;
  }

private:
  union {
    char dummy;
    T val;
  };
  bool active = false;
};

// NOTE: actually not needed for Root, but this is the only possible non-templated base type; Node and Root also need a common prev/next Base...
struct SignalConnectionBase {
  virtual ~SignalConnectionBase();
  virtual void remove_node() = 0;

  ::Connection *conns = {};
};

template <typename Sig>
struct SignalListBase;

template <typename Ret, typename... Args>
struct SignalListBase<Ret(Args...)> : SignalConnectionBase {
  SignalListBase() = default;
  SignalListBase(bool once) : once(once) { }
  SignalListBase(SignalListBase &&) = delete;  // TODO?

  using optional_Ret = typename std::conditional<!std::is_void<Ret>::value, simple_optional<Ret>, void>::type;

  virtual optional_Ret operator()(Args...) = 0;

  virtual void setNext(std::unique_ptr<SignalListBase> &node) {
    next.swap(node);
  }

  void remove_node() override {
    // assert(prev && next);
    next->prev = prev;

    std::unique_ptr<SignalListBase> tmp = std::move(next);
    prev->setNext(tmp);  // tmp now holds *this and keeps it alive until the current scope ends
    // ~SignalConnectionBase() will now disarm the connections
  }

  SignalListBase *prev = {};
  std::unique_ptr<SignalListBase> next;
  bool once = false;  // unused in SignalListRoot, but shall be accessible with only Base*
};

template <typename Sig, typename Fn, typename FnRet>
struct SignalListNode;

template <typename Ret, typename... Args, typename Fn, typename FnRet>
struct SignalListNode<Ret(Args...), Fn, FnRet> final : SignalListBase<Ret(Args...)> {
  SignalListNode(Fn&& fn, bool once)
    : SignalListBase<Ret(Args...)>(once), fn((Fn&&)fn)
  { }

  simple_optional<Ret> operator()(Args... args) override {
    return {fn(args...)};
  }

  Fn fn;
};

template <typename Ret, typename... Args, typename Fn>
struct SignalListNode<Ret(Args...), Fn, void> final : SignalListBase<Ret(Args...)> {
  SignalListNode(Fn&& fn, bool once)
    : SignalListBase<Ret(Args...)>(once), fn((Fn&&)fn)
  { }

  simple_optional<Ret> operator()(Args... args) override {
    fn(args...);
    return {};
  }

  Fn fn;
};

// generate a nice error:
template <typename... Args, typename Fn, typename FnRet>
struct SignalListNode<void(Args...), Fn, FnRet> final : SignalListBase<void(Args...)> {
  SignalListNode(Fn&& fn, bool once) {
    static_assert(std::is_void<FnRet>::value, "Function for Signal<void(...)> must not return a value");
  }
  void operator()(Args... args) override { throw 0; }
};

template <typename... Args, typename Fn>
struct SignalListNode<void(Args...), Fn, void> final : SignalListBase<void(Args...)> {
  SignalListNode(Fn&& fn, bool once)
    : SignalListBase<void(Args...)>(once), fn((Fn&&)fn)
  { }

  void operator()(Args... args) override {
    fn(args...);
  }

  Fn fn;
};

template <typename Sig, typename OnEmptyFn = void>
struct SignalListRoot final : SignalListRoot<Sig, void> {
  using base_t = SignalListBase<Sig>;

  SignalListRoot(OnEmptyFn& onempty)
    : onempty(onempty)
  { }

  void clear() {
    if (base_t::prev != this) {
      // SignalListRoot<Sig, void>::clear();
      base_t::next = std::move(base_t::prev->next);
      base_t::prev = this;
      onempty();
    }
  }

  void setNext(std::unique_ptr<base_t> &node) override {
    // base_t::setNext(node);
    base_t::next.swap(node);
    if (base_t::prev == this) { // (via remove_node) - or: (base_t::next.get() == this)
      onempty();
    }
  }

private:
  OnEmptyFn onempty;
};

template <typename Ret, typename... Args>
struct SignalListRoot<Ret(Args...), void> : SignalListBase<Ret(Args...)> {
  using base_t = SignalListBase<Ret(Args...)>;

  void clear() {
    base_t::next = std::move(base_t::prev->next);
    base_t::prev = this;
  }
  typename base_t::optional_Ret operator()(Args...) override {
    throw 0;
  }
  void remove_node() override {
    throw 0;
  }
//  using base_t::setNext;
};

enum class reduce_result_t : uint8_t {
  CONTINUE,
  REMOVE_HANDLER,
  STOP
};

struct reduce_void { // {{{
  reduce_result_t operator()(...) {
    return reduce_result_t::CONTINUE;
  }
  void get() { }
};
// }}}

template <typename T>
struct reduce_use_last { // {{{
  reduce_use_last(T init = {}) : last(std::move(init)) {}

  reduce_result_t operator()(T next = {}) {
    last = std::move(next);
    return reduce_result_t::CONTINUE;
  }

  T get() { return last; }

  T last;
};

template <>
struct reduce_use_last<void> : reduce_void { };
// }}}

} // namespace detail

struct Connection final {
  Connection()
    : node(), prev(), next()
  { }

  ~Connection() {
    if (node) {
      unlink();
    }
  }

  Connection(const Connection &rhs) = delete;
  Connection(Connection &&rhs)
    : Connection()
  {
    *this = std::move(rhs);
  }

//  Connection &operator=(const Connection &rhs) = delete;
  Connection &operator=(Connection &&rhs) {
    if (node) {
      unlink();
      node = nullptr; // "just in case"
    }

    node = move(rhs.node);
    prev = move(rhs.prev);
    next = move(rhs.next);

    if (node) {
      if (next) {
        next->prev = &next;
      }
      *prev = this;
    }

    return *this;
  }

  void disconnect() {
    if (node) {
      unlink();
      node->remove_node();
      node = nullptr;
    }
  }

  bool connected() const {
    return node;
  }

private:
  template <typename Sig, typename OnEmptyFn, typename DefaultRetvalFn>
  friend class Signal;
  friend struct detail::SignalConnectionBase;

  Connection(detail::SignalConnectionBase *node)
    : node(node), prev(&node->conns), next(node->conns) {
    node->conns = this;
    if (next) {
      *next->prev = this;
    }
  }

  void unlink() {
    // assert(node && prev);
    if (next) {
      next->prev = prev;
    }
    *prev = next;
  }

  template <typename T>
  T move(T &t, T&& new_val = {}) {
    T ret = std::move(t);
    t = (T&&)new_val;
    return ret;
  }

  detail::SignalConnectionBase *node;
  Connection **prev, *next;
};

inline detail::SignalConnectionBase::~SignalConnectionBase()
{
  while (conns) {
    conns->node = nullptr;
    conns = conns->next;
  }
}

//template <typename Sig, typename OnEmptyFn = void, typename DefaultRetvalFn = detail::reduce_void>
template <typename Sig, typename OnEmptyFn = void, typename DefaultRetvalFn = detail::reduce_use_last<void>>
class Signal;

template <typename Ret, typename... Args, typename OnEmptyFn, typename DefaultRetvalFn>
class Signal<Ret(Args...), OnEmptyFn, DefaultRetvalFn> final {
  using Sig = Ret(Args...);
  using base_t = detail::SignalListBase<Sig>;

  template <typename Fn>
  using RetOf = decltype(std::declval<Fn>()(std::declval<Args>()...));
public:
  Signal() = default;
  Signal(const Signal &) = delete;
  Signal &operator=(const Signal &) = delete;

  // template to avoid error when OnEmptyFn = void
  template <typename Fn = OnEmptyFn,
            typename = typename std::enable_if<std::is_same<Fn, OnEmptyFn>::value>::type>
  Signal(Fn& onempty)
    : root{new detail::SignalListRoot<Sig, OnEmptyFn>{onempty}}
  {
    root->next.reset(root);
    root->prev = root;
  }

  template <typename Fn = OnEmptyFn,
            typename = typename std::enable_if<std::is_same<Fn, OnEmptyFn>::value>::type>
  Signal(Fn&& onempty)
    : root{new detail::SignalListRoot<Sig, OnEmptyFn>{onempty}}
  {
    root->next.reset(root);
    root->prev = root;
  }

  ~Signal() {
    if (root) {
      root->next.reset();
    }
  }

  void clear() {
    if (root) {
      root->clear();
    }
  }

  bool empty() const {
    return (!root || root == root->prev);
  }

  template <typename Fn>
  Connection prepend(Fn&& fn, bool once = false) {
    auto node = new detail::SignalListNode<Sig, Fn, RetOf<Fn>>{(Fn&&)fn, once};
    ensure_root();
    insert_node(node, root->next);
    return {node};
  }

  template <typename Fn>
  Connection append(Fn&& fn, bool once = false) {
    auto node = new detail::SignalListNode<Sig, Fn, RetOf<Fn>>{(Fn&&)fn, once};
    ensure_root();
    insert_node(node, root->prev->next);
    return {node};
  }

  template <typename Fn>
  Connection connect(Fn&& fn, SignalFlags flags = {}) {
    return (flags & SIGNAL_PREPEND) ? prepend((Fn&&)fn, flags & SIGNAL_ONCE) : append((Fn&&)fn, flags & SIGNAL_ONCE);
  }

  template <typename ReduceRetvalFn = DefaultRetvalFn,
            typename = typename std::enable_if<!std::is_void<Ret>::value, ReduceRetvalFn>::type>
  auto emit(Args... args) -> decltype(((ReduceRetvalFn *)0)->get()) {
    ReduceRetvalFn ret;
    for (base_t *node = root ? root->next.get() : root; node != root; node = node->next.get()) {
      auto &&opt = (*node)(args...);
      const detail::reduce_result_t res = (opt) ? ret(*opt) : ret();
      if (res == detail::reduce_result_t::REMOVE_HANDLER || node->once) {
        node = node->prev;
        node->next->remove_node();
      }
      if (res == detail::reduce_result_t::STOP) {
        return ret.get();
      }
    }
    return ret.get();
  }

  template <typename ReduceRetvalFn = DefaultRetvalFn,
            typename = typename std::enable_if<std::is_void<Ret>::value, ReduceRetvalFn>::type>
  void emit(Args... args) {
    for (base_t *node = root ? root->next.get() : root; node != root; node = node->next.get()) {
      (*node)(args...);
      if (node->once) {
        node = node->prev;
        node->next->remove_node();
      }
    }
  }

private:
  detail::SignalListRoot<Sig, OnEmptyFn> *root = {};

  void ensure_root() {
    if (!root) { // i.e. via default ctor
      root = new detail::SignalListRoot<Sig, OnEmptyFn>{};
      root->next.reset(root);
      root->prev = root;
    }
  }

  void insert_node(base_t *node, std::unique_ptr<base_t> &atnext) {
    // assert(node && atnext);
    // assert(!node->prev && !node->next);

    node->prev = atnext->prev;
    atnext->prev = node;

    node->next = std::move(atnext);
    atnext.reset(node);
  }
};

