// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LETRA — src/nucleo/letra.hpp
// ══════════════════════════════════════════════════════════════════════════
// Busca a letra no LRCLIB e grava-a em `.lrc` ao lado do audio. A busca acontece
// UMA vez, no momento do download; escutar NUNCA toca a rede. É decisão do
// operador, e ella tem consequencia de desenho: o tocador lê arquivo, e não
// serviço, donde tocar sem internet é o caso ordinario e não a excepção.
//
// DOMÍNIO ......... o que se sabe da faixa (artista, titulo, album, duração), e
//                   o corpo que o LRCLIB devolveu.
// CONTRA-DOMÍNIO .. um arquivo `.lrc` ao lado do audio, ou nada, sem erro.
// INVARIANTE ...... letra ausente NÃO é falha. A maior parte do acervo de
//                   qualquer um não tem letra sincronizada, e tratar isso como
//                   erro faria o download parecer roto quando está inteiro.
// Q.E.D. .......... sendo puras a montagem da URL, o recorte do primeiro objecto
//                   e a leitura da resposta, a bateria afere-as sobre corpos
//                   escriptos á mão, sem tocar a rede.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace mysong::nucleo {

// A LETRA como o LRCLIB a dá: a sincronizada, com os carimbos de tempo, e a
// plana. Vazias as duas quer dizer que não ha letra, e não que houve erro.
struct Letra {
  std::string sincronizada;
  std::string plana;
};

// escapa_para_url — o que se põe n'um parametro de consulta. Espaço vira `%20`, e
// tudo o que não é letra, digito, hyphen, ponto, sublinhado ou til vira `%XX`.
// Sem isto, um titulo com `&` partiria a consulta em duas.
std::string escapa_para_url(std::string_view crua);

// url_da_busca — o `/api/search` do LRCLIB, com os campos que se tiverem. Usa-se
// o `search` e não o `get`: o `get` exige casamento EXACTO de artista, titulo,
// album e duração, e acerto exacto é raro n'um acervo que veio do YouTube.
std::string url_da_busca(std::string_view artista, std::string_view titulo);

// primeiro_objecto — o primeiro objecto de UM arranjo JSON, em texto. O
// `/api/search` devolve arranjo, e o jsonzinho d'esta Casa lê objecto PLANO de um
// nivel: recorta-se pois o primeiro, respeitando aspas e contra-barra, para que
// uma chave que contenha `}` não engane o recorte. Vazio quando não ha objecto.
std::string primeiro_objecto(std::string_view arranjo);

// le_resposta — a Letra que o corpo tras. Corpo que não seja objecto plano, ou que
// não traga letra alguma, dá Letra vazia: é resposta, e não erro.
Letra le_resposta(std::string_view corpo);

// caminho_do_lrc — o `.lrc` ao lado do audio: mesma pasta, mesmo nome, outra
// extensão. É onde o tocador o ha de procurar, e onde os outros tocadores o põem.
std::filesystem::path caminho_do_lrc(const std::filesystem::path& audio);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
