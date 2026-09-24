// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ESTALEIRO, src/nucleo/estaleiro.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. Uma tranca guarda TUDO o que os obreiros vêem, e nada d'esse
// estado se lê fóra d'ella: contador lido sem tranca é contador que a machina
// pode ter meio escripto.
//
// DOMÍNIO ......... as encommendas, e a obra que as cumpre.
// CONTRA-DOMÍNIO .. o andamento, e os arquivos que a obra deixou no disco.
// INVARIANTE ...... o obreiro incrementa em_curso_ DENTRO da tranca e ANTES de
//                   chamar a obra, e decrementa-o depois: donde o pico é o pico
//                   de verdade, e não uma amostra colhida no intervallo.
// Q.E.D. .......... fechando-se, a espera ABANDONA-SE e sómente as obras em voo
//                   se esperam: assim sahir do programa não fica pendurado n'uma
//                   fila de vinte baixas que ninguem mais vae ver.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/estaleiro.hpp"

#include <algorithm>
#include <utility>

namespace mysong::nucleo {

namespace {

// junta, acrescenta um pedaço á lista, com a virgula sómente quando ha o que
// separar. Existe para que a virgula não appareça no principio da linha.
void junta(std::string* dito, const std::string& pedaco) {
  if (!dito->empty()) *dito += ", ";
  *dito += pedaco;
}

// plural, «uma colhida», «duas colhidas». A concordancia faz parte do recado.
std::string plural(std::size_t quantas, const std::string& singular) {
  return std::to_string(quantas) + " " + singular + (quantas == 1 ? "" : "s");
}

}  // namespace

std::string texto_do_andamento(const Andamento& andamento) {
  // Estaleiro que nunca trabalhou não tem recado: cadeia vazia, e a tela cala-se.
  // Dizer «0 a baixar» seria occupar a linha da trilha com nada. Não ha guarda
  // apartada para esse caso, e é de proposito: nenhum dos ramos abaixo dispara
  // com os contadores todos a zero, donde a cadeia sahe vazia por construcção.
  // A guarda existia, e sahiu por ser codigo morto: a mutação que a apagava
  // sobrevivia á bateria, que é o signal de que ella nada guardava.
  std::string dito;
  if (andamento.em_curso > 0)
    junta(&dito, std::to_string(andamento.em_curso) + " a baixar");
  if (andamento.na_espera > 0)
    junta(&dito, std::to_string(andamento.na_espera) + " na espera");
  if (andamento.colhidas > 0) junta(&dito, plural(andamento.colhidas, "colhida"));
  if (andamento.falhadas > 0) junta(&dito, plural(andamento.falhadas, "falhada"));
  if (andamento.duvidosas > 0)
    junta(&dito, plural(andamento.duvidosas, "duvidosa"));
  // A razão do ultimo desfecho é APPENSO, e não recado por si: sem contador algum
  // a acompanhá-la, «(baixado)» sósinho na linha da trilha não diz de quê.
  if (!andamento.ultima.empty() && !dito.empty())
    dito += " (" + andamento.ultima + ")";
  return dito;
}

std::string texto_das_baixas(const Andamento& andamento) {
  std::string dito;
  std::size_t mostradas = 0;
  const auto acrescenta = [&dito, &mostradas](const RegistroDaBaixa& registro) {
    if (!dito.empty()) dito += "  |  ";
    dito += "#" + std::to_string(registro.id) + " " +
            std::string(nome_da_fonte(registro.fonte)) + " ";
    switch (registro.estado) {
      case EstadoDaBaixa::Aguardando: dito += "aguardando"; break;
      case EstadoDaBaixa::Preparando: dito += "preparando"; break;
      case EstadoDaBaixa::Baixando:
        dito += registro.porcentagem ? std::to_string(*registro.porcentagem) + "%"
                                      : "baixando...";
        break;
      case EstadoDaBaixa::Concluido: dito += "concluído"; break;
      case EstadoDaBaixa::Falhou: dito += "falhou"; break;
    }
    if (registro.estado == EstadoDaBaixa::Falhou && !registro.detalhe.empty())
      dito += " (" + registro.detalhe.substr(0, 36) + ")";
    ++mostradas;
  };
  for (const auto& registro : andamento.registros) {
    if (mostradas == 3) break;
    if (registro.estado != EstadoDaBaixa::Concluido &&
        registro.estado != EstadoDaBaixa::Falhou) acrescenta(registro);
  }
  for (auto i = andamento.registros.rbegin(); i != andamento.registros.rend(); ++i) {
    if (mostradas == 3) break;
    if (i->estado == EstadoDaBaixa::Concluido ||
        i->estado == EstadoDaBaixa::Falhou) acrescenta(*i);
  }
  if (andamento.registros.size() > mostradas)
    dito += "  +" + std::to_string(andamento.registros.size() - mostradas);
  return dito;
}

Estaleiro::Estaleiro(std::size_t obreiros, Obra obra) : obra_(std::move(obra)) {
  // Zero obreiro seria estaleiro que aceita encommenda e nunca a cumpre, que é
  // pior que erro: é silencio. Um, pelo menos.
  const std::size_t quantos = obreiros == 0 ? 1 : obreiros;
  obreiros_.reserve(quantos);
  for (std::size_t i = 0; i < quantos; ++i)
    obreiros_.emplace_back(&Estaleiro::obreiro, this);
}

Estaleiro::~Estaleiro() { fecha(); }

void Estaleiro::espera_a_fila() {
  std::unique_lock<std::mutex> chave(tranca_);
  sino_.wait(chave, [this] { return espera_.empty() && em_curso_ == 0; });
}

void Estaleiro::fecha() {
  {
    std::lock_guard<std::mutex> chave(tranca_);
    if (fechado_) return;  // fechado duas vezes: a segunda não junta os fios outra vez
    fechado_ = true;
    for (const auto& pedido : espera_)
      for (auto& registro : registros_)
        if (registro.id == pedido.identificador) {
          registro.estado = EstadoDaBaixa::Falhou;
          registro.detalhe = "cancelado ao fechar";
        }
    // A ESPERA abandona-se. Esperar por ella faria sahir do programa depender de
    // quantas baixas o operador encommendou, e ninguem espera meia hora para fechar
    // uma tela. A obra EM VOO espera-se, que essa já escreve no disco.
    espera_.clear();
  }
  sino_.notify_all();
  for (std::thread& fio : obreiros_)
    if (fio.joinable()) fio.join();
  obreiros_.clear();
}

void Estaleiro::encommenda(Pedido pedido) {
  {
    std::lock_guard<std::mutex> chave(tranca_);
    if (fechado_) return;  // estaleiro fechado não aceita obra nova
    pedido.identificador = proximo_id_++;
    RegistroDaBaixa registro;
    registro.id = pedido.identificador;
    registro.fonte = pedido.fonte;
    registro.titulo = pedido.titulo.empty() ? "URL" : pedido.titulo.substr(0, 24);
    registros_.push_back(std::move(registro));
    espera_.push_back(std::move(pedido));
  }
  sino_.notify_one();
}

Andamento Estaleiro::andamento() const {
  std::lock_guard<std::mutex> chave(tranca_);
  Andamento agora;
  agora.em_curso = em_curso_;
  agora.na_espera = espera_.size();
  agora.colhidas = colhidas_;
  agora.falhadas = falhadas_;
  agora.duvidosas = duvidosas_;
  agora.ultima = ultima_;
  agora.registros.assign(registros_.begin(), registros_.end());
  return agora;
}

void Estaleiro::limpa_recentes() {
  std::lock_guard<std::mutex> chave(tranca_);
  const auto fim = std::remove_if(registros_.begin(), registros_.end(),
      [](const RegistroDaBaixa& registro) {
    return registro.estado == EstadoDaBaixa::Concluido ||
           registro.estado == EstadoDaBaixa::Falhou;
  });
  registros_.erase(fim, registros_.end());
}

bool Estaleiro::colheu() {
  std::lock_guard<std::mutex> chave(tranca_);
  const bool houve = colheu_;
  colheu_ = false;  // CONSOME: a bandeira vale uma vez por colheita
  return houve;
}

bool Estaleiro::fechado() const {
  std::lock_guard<std::mutex> chave(tranca_);
  return fechado_;
}

std::size_t Estaleiro::pico() const {
  std::lock_guard<std::mutex> chave(tranca_);
  return pico_;
}

void Estaleiro::obreiro() {
  for (;;) {
    Pedido pedido;
    {
      std::unique_lock<std::mutex> chave(tranca_);
      sino_.wait(chave, [this] { return fechado_ || !espera_.empty(); });
      if (fechado_) return;
      pedido = std::move(espera_.front());
      espera_.pop_front();
      for (auto& registro : registros_)
        if (registro.id == pedido.identificador)
          registro.estado = EstadoDaBaixa::Preparando;
      // O incremento vae DENTRO da tranca e ANTES da obra: é o que faz do pico o
      // pico de verdade, e não uma amostra colhida no intervallo entre os dous.
      ++em_curso_;
      if (em_curso_ > pico_) pico_ = em_curso_;
    }
    // A OBRA corre FÓRA da tranca. Correndo dentro, dous obreiros nunca correriam
    // ao mesmo tempo e o limite de dous seria limite de um, dito por engano.
    std::filesystem::path ficou;
    pedido.noticia = [this, id = pedido.identificador](
        std::optional<int> porcentagem, std::string_view erro) {
      std::lock_guard<std::mutex> chave(tranca_);
      for (auto& registro : registros_) {
        if (registro.id != id) continue;
        if (!erro.empty()) registro.detalhe = std::string(erro.substr(0, 180));
        else {
          registro.estado = EstadoDaBaixa::Baixando;
          registro.porcentagem = porcentagem;
        }
        break;
      }
    };
    const Colheita fim = obra_(pedido, &ficou);
    {
      std::lock_guard<std::mutex> chave(tranca_);
      --em_curso_;
      ultima_ = std::string(razao_da_colheita(fim));
      for (auto& registro : registros_)
        if (registro.id == pedido.identificador) {
          registro.estado = fim == Colheita::Colhido ||
                            fim == Colheita::ColhidoDuvidoso ||
                            fim == Colheita::JaExiste
                                ? EstadoDaBaixa::Concluido : EstadoDaBaixa::Falhou;
          if (registro.detalhe.empty()) registro.detalhe = ultima_;
          break;
      if (fim == Colheita::Colhido) {
        ++colhidas_;
        colheu_ = true;
      } else if (fim == Colheita::ColhidoDuvidoso) {
        // Baixou pelo criterio de hoje, sem a gravação casada (issue #57): o
        // ARQUIVO ficou, donde a bandeira da colheita se levanta e a tela varre
        // o disco; mas conta-se por duvidosa, que o que ella pede é olho humano.
        ++duvidosas_;
        colheu_ = true;
      } else if (fim == Colheita::Duvidosa) {
        // DUVIDOSA não é falha, e conta-se á parte: faixa que não casou pede olho
        // humano, e dizer «falhou» faria o operador tentar outra vez o mesmo.
        ++duvidosas_;
      } else {
        ++falhadas_;
      }
    }
    // Acorda-se TODOS, e não um: quem espera pela fila espera n'este mesmo sino, e
    // um notify_one poderia acordar sómente um obreiro e deixá-lo dormir para sempre.
    sino_.notify_all();
  }
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
