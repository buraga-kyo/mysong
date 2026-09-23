#include <doctest/doctest.h>

#include <atomic>
#include <future>
#include <thread>

#include "tui/campainha.hpp"

namespace tui = mysong::tui;

TEST_CASE("campainha fechada acorda trabalhador sem executar pedido") {
  tui::Campainha campainha;
  std::atomic<int> chamadas{0};
  std::thread fio([&] {
    while (campainha.espera()) ++chamadas;
  });
  campainha.fecha();
  fio.join();
  CHECK(chamadas.load() == 0);
}

TEST_CASE("um pedido acorda o trabalhador uma vez") {
  tui::Campainha campainha;
  std::promise<void> executou;
  std::thread fio([&] {
    if (campainha.espera()) executou.set_value();
  });
  campainha.toca();
  executou.get_future().wait();
  campainha.fecha();
  fio.join();
  CHECK(true);
}

TEST_CASE("dous pedidos pendentes fundem-se n'uma execução") {
  tui::Campainha campainha;
  campainha.toca();
  campainha.toca();
  CHECK(campainha.espera());
  campainha.fecha();
  CHECK_FALSE(campainha.espera());
}

TEST_CASE("pedido durante trabalho fica para segunda execução") {
  tui::Campainha campainha;
  std::promise<void> começou;
  std::promise<void> continue_trabalho;
  std::shared_future<void> pode_continuar(continue_trabalho.get_future());
  std::atomic<int> chamadas{0};
  std::thread fio([&] {
    while (campainha.espera()) {
      const int qual = ++chamadas;
      if (qual == 1) {
        começou.set_value();
        pode_continuar.wait();
      } else {
        campainha.fecha();
      }
    }
  });
  campainha.toca();
  começou.get_future().wait();
  campainha.toca();
  continue_trabalho.set_value();
  fio.join();
  CHECK(chamadas.load() == 2);
}
