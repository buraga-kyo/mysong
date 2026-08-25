// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO NAVEGADOR — src/tui/navegador.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. Uma regra governa tudo: TODA mudança de vista passa por
// refaz_vista(), e é ella que apara o eleito. Donde não ha caminho por onde o
// eleito saia da lista, e o invariante é estructural e não vigilancia.
//
// DOMÍNIO ......... a bibliotheca, a secção, a trilha e o termo.
// CONTRA-DOMÍNIO .. a vista e o eleito.
// INVARIANTE ...... o eleito está dentro da vista, ou a vista é vazia e elle é
//                   zero. Uma funcção só o garante.
// Q.E.D. .......... havendo uma porta única para a vista, acrescentar secção é
//                   acrescentar um ramo, e não rever a aparadura em N logares.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/navegador.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace mysong::tui {

namespace {

// contem_sem_caixa — a comparação do filtro. Sem caixa, que o operador que busca
// «bach» não ha de escrever «Bach» para achar o que já vê na tela. Sem dobra de
// acento, de proposito: dobrar acento em UTF-8 pede taboa que esta Casa não tem,
// e prometter menos é melhor que prometter e falhar no «á» contra o «a».
bool contem_sem_caixa(const std::string& palheiro, const std::string& agulha) {
  if (agulha.empty()) return true;
  const auto baixa = [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  };
  std::string p, a;
  p.reserve(palheiro.size());
  a.reserve(agulha.size());
  for (const unsigned char c : palheiro) p += baixa(c);
  for (const unsigned char c : agulha) a += baixa(c);
  return p.find(a) != std::string::npos;
}

}  // namespace

// A PORTA UNICA da vista. Toda mudança de secção, de trilha ou de termo passa por
// aqui, e é aqui que o eleito se apara. Não ha segunda aparadura em logar algum:
// havendo duas, uma delas ficaria por corrigir no dia em que a regra mudasse.
void Navegador::refaz_vista() {
  vista_.clear();
  switch (secao_) {
    case Secao::Artistas:
      for (const std::string& nome : livraria_.artistas())
        if (contem_sem_caixa(nome, termo_)) vista_.push_back({nome, nome, 0, 0, {}});
      break;

    case Secao::Albuns:
      for (const std::string& nome : livraria_.albuns(trilha_.front()))
        if (contem_sem_caixa(nome, termo_)) vista_.push_back({nome, nome, 0, 0, {}});
      break;

    case Secao::Faixas:
      for (const nucleo::Faixa& faixa :
           livraria_.faixas_do_album(trilha_.front(), trilha_.back()))
        if (contem_sem_caixa(faixa.titulo, termo_))
          vista_.push_back({faixa.titulo, faixa.caminho, faixa.numero,
                            faixa.duracao, faixa.artista});
      break;

    case Secao::Busca:
      for (const nucleo::Faixa& faixa : livraria_.busca_faixa(termo_))
        vista_.push_back({faixa.titulo, faixa.caminho, faixa.numero,
                          faixa.duracao, faixa.artista});
      break;

    case Secao::Rede:
      // A UNICA secção que não pergunta á bibliotheca. A fonte é a lista que veio
      // de fóra, e o filtro applica-se sobre ella como sobre as outras.
      //
      // É d'aqui que vem a innocuidade de recarrega() n'esta secção: não havendo
      // consulta ao acervo n'este ramo, a varredura que conclua no meio de o
      // operador escolher um achado refaz a vista IDENTICA, e não lhe apaga a lista.
      for (const Linha& achado : rede_)
        if (contem_sem_caixa(achado.texto, termo_)) vista_.push_back(achado);
      break;
  }
  if (vista_.empty()) eleito_ = 0;
  else if (eleito_ >= vista_.size()) eleito_ = vista_.size() - 1;
}

std::size_t primeira_a_mostrar(std::size_t eleito, std::size_t quantas,
                               std::size_t altura,
                               std::size_t primeira_de_antes) {
  if (altura == 0 || quantas == 0) return 0;
  if (quantas <= altura) return 0;  // cabe tudo: não ha rolagem que fazer

  std::size_t primeira = primeira_de_antes;
  // Se a fatia de antes já não cabe na lista, encosta-se ao fim.
  if (primeira + altura > quantas) primeira = quantas - altura;
  // E rola-se o MENOS que baste para o eleito caber.
  if (eleito < primeira) primeira = eleito;
  else if (eleito >= primeira + altura) primeira = eleito - altura + 1;
  return primeira;
}

Navegador::Navegador(const nucleo::Biblioteca& livraria) : livraria_(livraria) {
  refaz_vista();
}

Secao Navegador::secao() const noexcept { return secao_; }
const std::vector<Linha>& Navegador::vista() const noexcept { return vista_; }
std::size_t Navegador::eleito() const noexcept { return eleito_; }
const std::vector<std::string>& Navegador::trilha() const noexcept {
  return trilha_;
}
const std::string& Navegador::termo() const noexcept { return termo_; }

void Navegador::desce() noexcept {
  if (vista_.empty()) return;
  if (eleito_ + 1 < vista_.size()) ++eleito_;
}

void Navegador::sobe() noexcept {
  if (eleito_ > 0) --eleito_;
}

void Navegador::ao_principio() noexcept { eleito_ = 0; }

void Navegador::ao_fim() noexcept {
  eleito_ = vista_.empty() ? 0 : vista_.size() - 1;
}

void Navegador::filtra(std::string termo) {
  termo_ = std::move(termo);
  eleito_ = 0;  // termo novo, lista nova: o eleito volta ao alto
  refaz_vista();
}

std::string Navegador::caminho_eleito() const {
  if (vista_.empty()) return {};
  if (secao_ != Secao::Faixas && secao_ != Secao::Busca) return {};
  return vista_[eleito_].chave;
}

bool Navegador::entra() {
  if (vista_.empty()) return false;
  const Linha degrau = vista_[eleito_];  // CÓPIA: refaz_vista limpa a vista
  switch (secao_) {
    case Secao::Artistas:
      trilha_ = {degrau.chave};
      secao_ = Secao::Albuns;
      break;
    case Secao::Albuns:
      trilha_ = {trilha_.front(), degrau.chave};
      secao_ = Secao::Faixas;
      break;
    case Secao::Faixas:
    case Secao::Busca:
      return true;  // já é faixa: quem chama manda tocar
    case Secao::Rede:
      // Achado da rede não é faixa, e entrar n'elle não é descer degrau algum:
      // quem chama pergunta pela url_eleita e manda baixar. Nada muda aqui.
      return false;
  }
  // O termo NÃO se herda ao descer: elle filtrava a lista de cima, e applicá-lo
  // á de baixo esconderia faixas por causa de uma busca que já se cumpriu.
  termo_.clear();
  eleito_ = 0;
  refaz_vista();
  return false;
}

bool Navegador::volta() {
  switch (secao_) {
    case Secao::Faixas:
      trilha_.resize(1);
      secao_ = Secao::Albuns;
      break;
    case Secao::Albuns:
    case Secao::Busca:
    case Secao::Rede:
      trilha_.clear();
      secao_ = Secao::Artistas;
      break;
    case Secao::Artistas:
      return false;  // já se está no alto
  }
  termo_.clear();
  eleito_ = 0;
  refaz_vista();
  return true;
}

void Navegador::recarrega() {
  // Conserva a secção e a trilha SE ellas ainda existirem no acervo novo. Um
  // artista que sahiu do disco não pode continuar a ser o titulo da tabella, e
  // insistir n'elle mostraria lista vazia sem dizer porque.
  if (!trilha_.empty()) {
    const std::vector<std::string> nomes = livraria_.artistas();
    if (std::find(nomes.begin(), nomes.end(), trilha_.front()) == nomes.end()) {
      trilha_.clear();
      secao_ = Secao::Artistas;
    }
  }
  if (secao_ == Secao::Faixas && trilha_.size() == 2) {
    const std::vector<std::string> albuns = livraria_.albuns(trilha_.front());
    if (std::find(albuns.begin(), albuns.end(), trilha_.back()) == albuns.end()) {
      trilha_.resize(1);
      secao_ = Secao::Albuns;
    }
  }
  refaz_vista();
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
