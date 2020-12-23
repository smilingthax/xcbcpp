#include "../signals.h"
#include <stdio.h>

// g++ -Wall -o test_signals test_signals.cpp

struct foo {
  template <typename ParentSig>
  explicit foo(ParentSig &sig)
    : conn(sig.connect(*this))
//    : conn(sig.connect(std::move(*this)))
  { }

//  foo(foo&&) = default;

  void operator()(int x) {
    printf("%d\n", x);
  }

  Connection conn;
};


int main()
{
#if 0
//std::unique_ptr<Connection> conn2;
  Signal<void(int)> sig;

auto y=[&sig](){ printf("empty\n"); }; Signal<int(int), decltype(y)&> sig3{y};
auto x=[&sig](){ printf("empty\n"); }; Signal<int(int), decltype(x)> sig2{x};
sig2.emit(5);

  auto conn = sig.append([&sig](int i) {
    printf("hi %d\n", i);
  });
auto conn2 = sig.append([](int i) { printf("ho %d\n", i); });
//conn2.reset(new Connection(std::move(conn)));
//auto conn2 = std::move(conn);

sig.emit(3);
//sig.clear();

//  conn.disconnect();
//  conn.disconnect();
//  conn2.disconnect();
//  conn2->disconnect();

#else
  Signal<void(int)> sig;

  struct bla {
    bla() = default;
    bla(bla &&) = default;
    void operator()(int i) { printf("hx %d\n", i); };
  };
sig.connect(bla{});

  foo f{sig};
auto conn2 = sig.append([](int i) { printf("ho %d\n", i); });
//auto y=[&sig](int i){ printf("hi %d\n", i); }; sig.connect(y); // sig.connect(std::move(y));

sig.emit(3);

#endif

  return 0;
}

