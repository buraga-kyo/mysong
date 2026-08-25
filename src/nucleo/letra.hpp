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
#include <vector>

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

// ── A LETRA NA TELA (issue #15) ─────────────────────────────────────────────

// Uma LINHA de letra com o seu instante. `tempo` em segundos, com os centesimos
// que o `.lrc` traz.
struct LinhaDaLetra {
  double tempo = 0.0;
  std::string texto;
};

// analysa_lrc — as linhas de um `.lrc`, em ordem de tempo. Linha sem carimbo
// ignora-se; carimbo sem texto CONSERVA-SE, com texto vazio, porque é assim que o
// LRCLIB marca o silencio entre estrophes e é isso que faz a linha anterior sahir
// da tela na hora certa.
//
// Um carimbo pode trazer MAIS DE UM tempo (`[00:11.00][01:23.00] refrão`), que é
// como o fórmato diz «esta mesma linha repete-se»: sahem duas linhas.
std::vector<LinhaDaLetra> analysa_lrc(std::string_view texto);

// linha_corrente — o indice da linha que vale n'um instante, ou menos um antes da
// primeira. As linhas HÃO DE vir ordenadas, que é como analysa_lrc as devolve.
int linha_corrente(const std::vector<LinhaDaLetra>& linhas, double posicao);

// le_lrc_do_disco — as linhas do `.lrc` que estiver ao lado do audio. Vazio quando
// não ha arquivo, e isso não é erro: a maior parte do acervo não tem letra.
std::vector<LinhaDaLetra> le_lrc_do_disco(const std::filesystem::path& audio);

// ── E AGORA O QUE TOCA A REDE. Uma funcção só, e no fim.

// busca_letra — pergunta ao LRCLIB. FALSO quando a rede não respondeu; letra
// vazia com verdadeiro quer dizer que respondeu e não ha letra, que é caso
// ordinario e não erro. O prazo é de OITO segundos: quem baixa uma faixa não ha de
// esperar por um serviço de letra mais do que isso.
bool busca_letra(std::string_view artista, std::string_view titulo,
                 Letra* letra);

// grava_lrc — escreve a letra sincronizada em `.lrc` ao lado do audio. Não grava a
// PLANA: `.lrc` é fórmato de letra com tempo, e pôr letra sem tempo n'um `.lrc`
// faria todo tocador do mundo mostrar a musica inteira n'uma linha. Falso quando
// não ha letra sincronizada, e ahi arquivo algum se cria.
bool grava_lrc(const std::filesystem::path& audio, const Letra& letra);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
