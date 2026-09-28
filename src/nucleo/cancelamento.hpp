#pragma once
#include <atomic>
#include <memory>
#include <curl/curl.h>

namespace mysong::nucleo {
// Cada fio conserva a bandeira da sua baixa, inclusive nas chamadas aninhadas.
inline thread_local std::shared_ptr<std::atomic_bool> interrupcao_corrente;
inline bool baixa_interrompida() {
  return interrupcao_corrente && interrupcao_corrente->load();
}
// O escopo restitue o contexto anterior mesmo quando a obra lança excepção.
struct EscopoDaBaixa {
  std::shared_ptr<std::atomic_bool> anterior = interrupcao_corrente;
  explicit EscopoDaBaixa(std::shared_ptr<std::atomic_bool> bandeira) {
    interrupcao_corrente = std::move(bandeira);
  }
  ~EscopoDaBaixa() { interrupcao_corrente = std::move(anterior); }
};
// Assignatura do libcurl: um pedido de parada aborta a transferencia corrente.
inline int consulta_interrupcao(void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
  return baixa_interrompida() ? 1 : 0;
}
}  // namespace mysong::nucleo
