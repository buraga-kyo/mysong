// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO COVER ART ARCHIVE — src/nucleo/caa.hpp
// ══════════════════════════════════════════════════════════════════════════
// A capa que falta ao acervo que JÁ está no disco (issue #83). A issue #81
// consertou a baixa nova: o yt-dlp embute a miniatura. Esta peça cuida do
// resto: por ORDEM do operador, casa a faixa sem capa no MusicBrainz pelos
// metadados que ella tem (faixa antiga não traz ISRC), pede a arte ao Cover
// Art Archive, que é o irmão do MB indexado pela RELEASE, e embute-a na
// etiqueta pelo mesmo quadro APIC da #81, que sobrevive a mover o arquivo.
//
// DOMÍNIO ......... faixas do índice, e os corpos e octetos que a rede dá.
// CONTRA-DOMÍNIO .. arte embutida, e desfechos NOMEADOS, faixa a faixa.
// INVARIANTE ...... o acervo não se toca sem ordem; negativo DEFINITIVO
//                   lembra-se fóra do índice (que a varredura reconstroe);
//                   o transitorio não se lembra; e o recuo pára a caça toda.
// Q.E.D. .......... a rede entra por parametro (a Consulta da casa), donde a
//                   bateria afere a caça inteira com corpos de mentira.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#include "nucleo/musicbrainz.hpp"

// O punho do SQLite, declarado adiante e no escopo global, pela mesma razão
// da bibliotheca: quem nos inclue não herda o sqlite3.h que não pediu.
struct sqlite3;

namespace mysong::nucleo {

// url_da_capa — a capa da FRENTE da release, na miniatura de quinhentos
// pixels (RULINGS R4 da issue). O front-500, e não o /front original: o
// original pode ser PNG de varios mega-octetos, e embutir isso em cada MP3
// incha o acervo para um painel que pinta meio-bloco de vinte collunas.
// A miniatura do CAA sahe JPEG, e o 307 que ella responde o curl já segue.
std::string url_da_capa(std::string_view release_mbid);

// O DESFECHO da resposta do CAA. QUATRO, e não os tres do MB, porque aqui o
// 404 tem sentido PROPRIO: é o CAA a dizer que a release não tem capa, que é
// resposta definitiva e se LEMBRA. A rede muda e o 5xx são Transitoria, que
// amanhã podem responder e por isso não se lembram; e o Recuo pára a caça.
enum class DesfechoDaCapa { Achada, SemCapa, Transitoria, Recuo };

// desfecho_da_capa — a leitura PURA do que consulta_mb_com_estado devolveu,
// para que a bateria a afira sem rede alguma.
DesfechoDaCapa desfecho_da_capa(DesfechoMB desfecho, long estado_http);

// ── A CAÇA DE UMA FAIXA ─────────────────────────────────────────────────────

// O DESFECHO da caça, faixa a faixa. Todo caso tem nome, pela regra da
// Colheita: «falhou» não diz ao operador se ha de consertar etiqueta, esperar
// a rede voltar, ou aceitar que a capa não existe. Os quatro primeiros
// decidem-se em casa, sem requisição; os demais contam a rede.
enum class CacaDeCapa {
  Embutida,        // a arte está na etiqueta, e o painel a verá
  JaTinha,         // havia capa, ao lado ou embutida: rede nem se tocou
  JaProcurada,     // corrida anterior já decidiu; a memoria poupa a rede
  ForaDoAlcance,   // não é MP3 que a taglib aceite: só ID3v2 se embute (R7)
  SemMetadado,     // sem titulo ou sem duração: sem crivo não ha casamento
  Duvidosa,        // o MB respondeu e não casou com confiança; lembra-se
  SemCapa,         // o CAA disse 404 para a release casada; lembra-se
  RedeFalhou,      // falha passageira: diz-se, e NÃO se lembra
  Recuo,           // 429 ou 503: a corrida inteira ha de parar aqui
  FalhouAEscripta, // a arte veio e a etiqueta não se deixou escrever
};

// palavra_da_caca — a linha que o relato mostra. Switch exhaustivo: desfecho
// novo sem palavra não compila.
std::string_view palavra_da_caca(CacaDeCapa desfecho);
inline constexpr int kVersaoDaMemoria = 1;

// O que se LEMBRA de uma faixa procurada. SÓ desfecho definitivo tem nome
// aqui: transitorio e recuo não entram na memoria de proposito, que lembrar
// queda de rede seria carimbar o acervo de «sem capa» por avaria de um dia.
enum class Procurada { SemCapa, Duvidosa };

// palavra_da_procurada — a palavra que se assenta no banco e se diz no
// relato. Vive aqui pela regra do razao_da_colheita: desfecho novo sem
// palavra não compila.
std::string_view palavra_da_procurada(Procurada procurada);

// A MEMORIA DAS PROCURADAS: `capas.sqlite3` AO LADO do índice, e nunca dentro
// d'elle, pela razão exacta do rol (RULINGS R5): o índice reconstroe-se a
// cada varredura por temporario e rename, e taboa lá dentro morreria na
// primeira. A chave é o CAMINHO da faixa (a identidade do índice; movida,
// procura-se outra vez). O banco ENTRA POR PARAMETRO: bateria em temporario.
class MemoriaDeCapas {
 public:
  explicit MemoriaDeCapas(std::filesystem::path banco);
  ~MemoriaDeCapas();

  MemoriaDeCapas(const MemoriaDeCapas&) = delete;
  MemoriaDeCapas& operator=(const MemoriaDeCapas&) = delete;

  bool aberta() const noexcept;
  int versao() const noexcept;

  // ja_procurada — corrida anterior já decidiu esta faixa? É o que poupa a
  // rede: quem já foi sem-capa ou duvidosa não volta á fila.
  bool ja_procurada(std::string_view caminho) const;

  // lembra — assenta o desfecho definitivo, com o relogio de agora. Falso
  // quando o banco recusou; caminho vazio não se assenta.
  bool lembra(std::string_view caminho, Procurada procurada);

  std::size_t quantas() const;  // serve á prova e á somma do relato

 private:
  std::filesystem::path banco_;
  sqlite3* punho_ = nullptr;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
