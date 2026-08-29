// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA FILA — src/nucleo/fila.hpp
// ══════════════════════════════════════════════════════════════════════════
// Guarda a ORDEM que o cliente definir, e anda por ella nos dous sentidos. É
// fila NOSSA, e não playlist do mpv: delegar a ordem ao mpv seria entregar-lhe
// o governo d'ella, e com o governo o poder de a mudar por conta propria.
//
// DOMÍNIO ......... caminhos de arquivo, na ordem em que se juntaram.
// CONTRA-DOMÍNIO .. a faixa corrente, e resposta booleana a cada passo.
// INVARIANTE ...... o indice aponta SEMPRE para faixa existente, ou então a
//                   fila está vazia e não ha para onde apontar. Não ha indice
//                   pendurado: proxima() no ultimo e anterior() no primeiro
//                   devolvem falso e NÃO movem nada, salvo com o repetir em
//                   «todas», que é quando a fila envolve de proposito. E com o
//                   embaralhado ligado, ordem_ é permutação de TODA a fila e
//                   indice_ é sempre ordem_[passo_].
// Q.E.D. .......... sendo o indice nosso e a resposta booleana, a prova corre
//                   sem mpv, sem placa de som e sem arquivo em disco: os
//                   caminhos são meras cadeias, e a fila jamais os abre.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

// O REPETIR, em TRES valores. Os nomes são os do MPRIS, e a correspondencia
// fica lavrada em api/unidades.cpp: Nenhuma é None, Uma é Track, Todas é
// Playlist. Tres valores, e não um booleano com bandeira ao lado: «uma» e
// «todas» fazem cousas differentes, e bandeira que muda de sentido conforme
// outra bandeira é o que se lê errado n'um switch.
enum class Repeticao { Nenhuma, Uma, Todas };

// nome_da_repeticao — o nome na lingua d'esta Casa, que é a que o socket fala.
inline std::string_view nome_da_repeticao(Repeticao modo) noexcept {
  switch (modo) {
    case Repeticao::Uma: return "uma";
    case Repeticao::Todas: return "todas";
    case Repeticao::Nenhuma: break;
  }
  return "nenhuma";
}

class Fila {
 public:
  // Junta ao fim, preservando a ordem de chegada.
  void junta(std::string caminho);

  bool vazia() const noexcept;
  std::size_t tamanho() const noexcept;

  // O indice da faixa corrente. Sem sentido em fila vazia, e por isso a
  // vazia() se pergunta primeiro.
  std::size_t indice() const noexcept;

  // A faixa corrente. Cadeia vazia quando a fila está vazia — resposta, e não
  // erro. A vista vale enquanto a fila não se alterar.
  std::string_view corrente() const noexcept;

  bool proxima() noexcept;
  bool anterior() noexcept;
  bool ir_para(std::size_t alvo) noexcept;

  void esvazia() noexcept;

  // A vista INTEIRA, na ordem guardada. Vale enquanto a fila não se alterar,
  // como a de corrente(); quem precise levá-la, que a copie.
  const std::vector<std::string>& todas() const noexcept;

  // ── OS DOUS MODOS (issue #62), e são INDEPENDENTES: um sorteia a ordem, o
  // outro governa o que succede na borda.
  //
  // NÃO SOBREVIVEM AO FECHAR O PROGRAMA, e é decisão declarada, não esquecimento:
  // arquivo de estado algum se lê nem se escreve, aqui ou em logar nenhum, e o
  // tocador aberto de novo nasce com os dous desligados. Quem vier accrescentar
  // persistencia ha de a pedir ao usuario primeiro; a issue #67 traz configuração
  // de LEITURA APENAS, e ella não guarda modo de reprodução.
  //
  // O EMBARALHAR é PERMUTAÇÃO QUE ESGOTA. Ao ligar, sorteia-se UMA ordem de
  // toda a fila, com a faixa corrente no PRINCIPIO d'ella, e anda-se por essa
  // ordem: faixa alguma torna antes de todas terem tocado. Esgotada, ella NÃO
  // se re-sorteia. Ao desligar, a fila volta á ordem de chegada e a faixa
  // corrente NÃO troca: o indice segue a FAIXA, e não a posição.
  void embaralhar(bool ligado);
  bool embaralhado() const noexcept;

  // A permutação, em assentos da ordem de chegada. Vazia quando se não
  // embaralha. Sahe para que a prova a inspeccione, em vez de a adivinhar.
  const std::vector<std::size_t>& ordem() const noexcept;

  void repetir(Repeticao modo) noexcept;
  Repeticao repeticao() const noexcept;

 private:
  std::vector<std::string> faixas_;
  std::size_t indice_ = 0;
  bool embaralhado_ = false;
  std::vector<std::size_t> ordem_;
  std::size_t passo_ = 0;  // o assento corrente DENTRO de ordem_
  Repeticao repeticao_ = Repeticao::Nenhuma;
  std::mt19937 sorteio_{std::random_device{}()};
};

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
