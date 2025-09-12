#include <inttypes.h>
#include <unistd.h>

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

#include "Halo.hpp"

using namespace std;
using namespace HALO;
int n = 8;
void insert_size_t() {
  // key_type=size_t, value_type=size_t

  PM_PATH = "/mnt/pmem/Halo_sizet/";
  Halo<size_t, size_t> halo(1024);
  // Halo<std::string, std::string> halo(32 * 1024);

  for (size_t i = 0; i < n; i++) {
    Pair_t<size_t, size_t> p(i, i);
    // Pair_t<std::string, std::string> p(reinterpret_cast<char *>(&i), 8,
    //                                    reinterpret_cast<char *>(&i), 8);
    halo.Insert(p);
  }
  for (size_t i = 0; i < n; i++) {
    Pair_t<std::size_t, std::size_t> p;

    p.set_key(i);

    // Pair_t<std::string, std::string> p;
    // rp[i].set_key(reinterpret_cast<char *>(&i), 8);

    halo.Get(p);
    cout << p.str_key() << "-" << p.value() << std::endl;

    Pair_t<size_t, size_t> p2(i, p.value() + 1);
    halo.Update(p2);

    // cout << *reinterpret_cast<size_t *>(p.key()) << "-"
    //  << *reinterpret_cast<size_t *>(&p.value()[0]) << endl;
  }
  std::cout << "==============Update=====================" << std::endl;

  for (size_t i = 0; i < n; i++) {
    Pair_t<std::size_t, std::size_t> p;
    p.set_key(i);
    halo.Get(p);
    cout << p.str_key() << "-" << p.value() << std::endl;
  }
}

void batch_insert() {
  // key_type=size_t, value_type=size_t, batch

  PM_PATH = "/mnt/pmem/Halo_sizet_batch/";
  Halo<size_t, size_t> halo(1024);

  Pair_t<size_t, size_t> p[n];
  int results[n];
  for (size_t i = 0; i < n; i++) {
    p[i] = Pair_t<size_t, size_t>(i, i);
  }

  // batch insert
  halo.Insert(p, results, n);

  // check insert result
  for (size_t i = 0; i < n; i++) {
    if (results[i] != DONE)
      std::cerr << "item " << i << "insert failed." << std::endl;
  }

  // ready for batch get
  for (size_t i = 0; i < n; i++) {
    p[i].set_empty();
    p[i].set_key(i);
  }

  // batch get
  halo.Get(p, n);

  // chect get result and update it
  for (size_t i = 0; i < n; i++) {
    cout << p[i].str_key() << "-" << p[i].value() << std::endl;
    Pair_t<size_t, size_t> p2(i, p[i].value() + 1);
    halo.Update(p2);
  }
  std::cout << "==============Update=====================" << std::endl;
  // check updated items
  for (size_t i = 0; i < n; i++) {
    Pair_t<std::size_t, std::size_t> p;
    p.set_key(i);
    halo.Get(p);
    cout << p.str_key() << "-" << p.value() << std::endl;
  }
}

void insert_varlen() {
  // key_type=std::string, value_type=std::string
  PM_PATH = "/mnt/pmem/Halo_string/";

  Halo<std::string, std::string> halo(32 * 1024);

  for (size_t i = 0; i < n; i++) {
    std::string s = std::to_string(i);
    Pair_t<std::string, std::string> p(s, s);
    halo.Insert(p);
  }
  for (size_t i = 0; i < n; i++) {
    std::string s = std::to_string(i);
    Pair_t<std::string, std::string> p;
    p.set_key(s);

    auto r = halo.Get(p);
    if (r == true)
      cout << p.str_key() << "-" << p.value() << endl;
    else
      cout << "key not found" << endl;
  }
}
void minibenchmark() {
  PM_PATH = "/mnt/pmem/Halo_bench/";
  Halo<size_t, size_t> halo(16 * 1024 * 1024);
  std::vector<std::thread> t;
  Timer t1;
  t1.start();
  for (size_t i = 0; i < 26; i++) {
    t.emplace_back(
        [](Halo<size_t, size_t> *h, int s) {
          int r[128];
          for (size_t i = s; i < 200000000; i += 26) {
            Pair_t<size_t, size_t> p(i, i);
            h->Insert(p, &r[i % 26]);
          }
          h->wait_all();
        },
        &halo, i);
  }
  for (size_t i = 0; i < 26; i++) {
    t[i].join();
  }

  auto tt = t1.elapsed<std::chrono::milliseconds>();
  t.clear();
  printf("Throughput: run, %f Mops/s\n",
         ((200000000 * 1.0) / 1000000) / (tt / 1000));
  Pair_t<size_t, size_t> *ps = new Pair_t<size_t, size_t>[200000000];
  t1.start();
  for (size_t i = 0; i < 26; i++) {
    t.emplace_back(
        [ps](Halo<size_t, size_t> *h, int s) {
          for (size_t i = s; i < 200000000; i += 26) {
            ps[i].set_key(i);
            h->Get(&ps[i]);
          }
          h->wait_all();
        },
        &halo, i);
  }
  for (size_t i = 0; i < 26; i++) {
    t[i].join();
  }
  tt = t1.elapsed<std::chrono::milliseconds>();

  printf("Throughput: run, %f Mops/s\n",
         ((200000000 * 1.0) / 1000000) / (tt / 1000));
}
int main(int argc, char *argv[]) {
  std::cout << "==============Insert size_t=====================" << std::endl;
  insert_size_t();
  std::cout << "==============Recover=====================" << std::endl;
  insert_size_t();
  std::cout << "==============Insert(batch)size_t====================="
            << std::endl;
  batch_insert();
  std::cout << "==============Insert varlen=====================" << std::endl;
  insert_varlen();
  minibenchmark();
  return 0;
}