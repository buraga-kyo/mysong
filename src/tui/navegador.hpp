// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO NAVEGADOR — src/tui/navegador.hpp
// ══════════════════════════════════════════════════════════════════════════
// A máquina de estados da navegação: em que secção se está, o que está á vista,
// qual a linha eleita, e o que acontece a cada ordem. Não pinta, não conhece
// FTXUI, não conhece Tocador. Quem pinta é a janella; quem toca é o nucleo.
//
// DOMÍNIO ......... uma Bibliotheca aberta (emprestada), e as ordens do
//                   operador: sobe, desce, entra, volta, filtra.
// CONTRA-DOMÍNIO .. a lista do que está á vista, o indice do eleito, e o
//                   caminho da faixa quando o operador manda tocar.
// INVARIANTE ...... o eleito está SEMPRE dentro da lista, ou a lista é vazia e
//                   elle é zero. Não ha ordem que o ponha fóra, e por isso quem
//                   pinta nunca ha de conferir o limite.
// Q.E.D. .......... sendo a navegação estado puro sobre consultas de leitura, a
//                   bateria percorre o caminho inteiro (Artistas, um artista, um
//                   album, uma faixa) sem erguer terminal e sem tocar som.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "nucleo/biblioteca.hpp"

namespace mysong::tui {

// As secções da barra lateral. A ordem é a do mockup, e é ella que a barra
// mostra de alto a baixo.
enum class Secao { Artistas, Albuns, Faixas, Busca };

// Uma LINHA do que está á vista. O texto é o que se mostra; a `chave` é o que a
// ordem `entra` consome, e nem sempre são a mesma cousa: o album mostra-se pelo
// nome e entra-se n'elle pelo par artista e album.
struct Linha {
  std::string texto;
  std::string chave;
  int numero = 0;      // sómente em faixas; zero é «sem numero»
  int duracao = 0;     // sómente em faixas, em segundos
  std::string autor;   // sómente em faixas: o artista, para a columna do meio
};

// O NAVEGADOR. A bibliotheca é EMPRESTADA e vive mais que elle: o navegador não a
// possue, e por isso não a fecha nem a reabre ás escondidas.
class Navegador {
 public:
  explicit Navegador(const nucleo::Biblioteca& livraria);

  Navegador(const Navegador&) = delete;
  Navegador& operator=(const Navegador&) = delete;

  Secao secao() const noexcept;
  const std::vector<Linha>& vista() const noexcept;
  std::size_t eleito() const noexcept;

  // A trilha do que se atravessou, para o titulo da tabella: vazia em Artistas,
  // com o artista em Albuns, com artista e album em Faixas.
  const std::vector<std::string>& trilha() const noexcept;

  void desce() noexcept;
  void sobe() noexcept;
  void ao_principio() noexcept;
  void ao_fim() noexcept;

  // entra — desce um degrau. VERDADEIRO quando o degrau era uma FAIXA, que é o
  // signal para quem chama mandar tocar; nesse caso `caminho_eleito` diz qual.
  bool entra();

  // volta — sobe um degrau. Falso quando já se está no alto, e ahi nada muda.
  bool volta();

  // O filtro. Cadeia vazia limpa-o. Filtra o que está Á VISTA, e não o acervo:
  // é o que o mockup mostra, e é o que o operador espera de uma barra de busca
  // que vive por cima de uma lista.
  void filtra(std::string termo);
  const std::string& termo() const noexcept;

  std::string caminho_eleito() const;

  // Relê o acervo da bibliotheca, conservando a secção e a trilha se ainda
  // existirem. Chama-se depois de a varredura concluir.
  void recarrega();

 private:
  void refaz_vista();

  const nucleo::Biblioteca& livraria_;
  Secao secao_ = Secao::Artistas;
  std::vector<Linha> vista_;
  std::vector<std::string> trilha_;
  std::string termo_;
  std::size_t eleito_ = 0;
};

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
