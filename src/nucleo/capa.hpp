// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA CAPA — src/nucleo/capa.hpp
// ══════════════════════════════════════════════════════════════════════════
// Acha a capa do album e converte-a em texto de terminal pelo `chafa`, em
// MEIO-BLOCO. Meio-bloco é o TECTO d'esta machina, e não escolha preguiçosa: o
// operador corre Alacritty dentro de tmux, e protocolo de imagem algum atravessa o
// tmux. Escolher sixel ou kitty seria pintar o que elle não veria.
//
// DOMÍNIO ......... o caminho de uma faixa, e a geometria do painel.
// CONTRA-DOMÍNIO .. linhas de texto com escapes de cor, prontas a pintar; ou
//                   nada, e ahi quem chama desenha o marcador.
// INVARIANTE ...... converter é CARO, e por isso o resultado guarda-se por album
//                   E por tamanho. Duas faixas do mesmo album pedem a mesma capa
//                   uma vez só; e redimensionar o terminal pede outra, porque a
//                   arte tem de encher o painel novo.
// Q.E.D. .......... sendo puras a busca do arquivo ao lado e a chave do cache, a
//                   bateria afere-as sem chamar o chafa e sem imagem alguma.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace mysong::nucleo {

// Os nomes de arquivo de capa que se procuram ao lado do album, na ORDEM em que se
// preferem. A lista é fechada, e a ordem é a convenção que os ripadores usam.
const std::vector<std::string>& nomes_de_capa();

// capa_ao_lado — o arquivo de capa que estiver na pasta da faixa, pela ordem dos
// nomes preferidos. Vazio quando não ha nenhum. Não desce nem sobe: capa de album
// vive junto das faixas d'elle, e ir buscá-la mais longe traria a do album vizinho.
std::filesystem::path capa_ao_lado(const std::filesystem::path& faixa);

// chave_do_cache — o que identifica um render. É a PASTA da faixa mais o tamanho,
// e não o caminho da faixa: as faixas de um album partilham a capa, e usar o
// caminho faria converter uma vez por faixa em vez de uma por album.
std::string chave_do_cache(const std::filesystem::path& faixa,
                           std::size_t collunas, std::size_t linhas);

// argumentos_do_chafa — o que se corre. Meio-bloco fixado, e a geometria em
// collunhas por linhas.
std::vector<std::string> argumentos_do_chafa(const std::filesystem::path& imagem,
                                             std::size_t collunas,
                                             std::size_t linhas);

// A CAPA renderizada: as linhas prontas a pintar, com os escapes dentro.
struct CapaPintada {
  std::vector<std::string> linhas;
  bool achada = false;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
