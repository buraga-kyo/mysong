// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO CATALOGO, src/nucleo/catalogo.hpp
// ══════════════════════════════════════════════════════════════════════════
// A playlist publica do Spotify lida como CATALOGO: titulo, artista, ordem e
// duração. O audio NÃO vem d'ella; vem do YouTube, pelo yt-dlp da issue #11.
//
// A FRONTEIRA é declarada, e não é technica: decifrar ou ripar o stream do Spotify
// é quebrar protecção technica de um serviço, e esta obra não o faz. O que se lê é
// a pagina publica de embutir, que o proprio Spotify serve a quem a peça sem chave
// nem conta alguma, e o que se tira d'ella é METADADO. É o methodo do spotdl.
//
// Por que a pagina de embutir, e não a pagina da playlist: a segunda monta a lista
// por script depois de carregar, e o corpo que ella entrega não a tras. A de
// embutir tras a lista inteira n'um bloco de JSON, que é o que se lê.
//
// DOMÍNIO ......... uma URL de playlist do Spotify, e o corpo que ella devolveu.
// CONTRA-DOMÍNIO .. o nome da lista, e as faixas com titulo, artista, ordem e
//                   duração em milesimos.
// INVARIANTE ...... faixa sem TITULO não sahe da leitura: ella seria linha que o
//                   operador elege e que não se pode buscar.
// Q.E.D. .......... sendo puras a extracção do identificador, a leitura do corpo e
//                   o casamento pela duração, a bateria afere as tres contra corpo
//                   escripto á mão, sem tocar a rede.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

// Uma FAIXA do catalogo. A duração vem em MILESIMOS porque é assim que o Spotify a
// dá, e converter aqui perderia a precisão que o casamento usa.
struct FaixaDoCatalogo {
  std::string titulo;
  std::string artista;
  int numero = 0;       // a posição na lista, contada de um
  int duracao_ms = 0;   // zero é «não disse»
  // O identificador do track no Spotify, tirado do `uri` da pagina (issue #57): é
  // por elle que o MusicBrainz acha a gravação e o ISRC. Vazio quando o uri falta
  // ou não é de track (episodio, arquivo local), e a faixa fica na lista assim mesmo.
  std::string id_do_track;
};

// O CATALOGO inteiro: o nome da lista, e as faixas na ordem d'ella.
struct Catalogo {
  std::string nome;
  std::vector<FaixaDoCatalogo> faixas;
};

// id_da_playlist, o identificador que a URL ou o URI tras. Aceita as tres fórmas
// que o Spotify dá: `https://open.spotify.com/playlist/<id>?si=...`,
// `spotify:playlist:<id>` e a de embutir. Vazio quando não ha identificador.
std::string id_da_playlist(std::string_view crua);

// url_do_embed, a pagina de embutir d'esse identificador.
std::string url_do_embed(std::string_view id);

// nome_da_lista, o nome que a pagina dá á playlist. É elle que vae por ALBUM nas
// etiquetas, e fica declarado por que: a pagina de embutir não publica album algum,
// e o disco de onde a faixa sahiu o Spotify não dá sem chave nem conta.
std::string nome_da_lista(std::string_view corpo);

// le_catalogo, o catalogo que o corpo da pagina tras. Corpo que não traga a lista
// dá catalogo vazio: é resposta, e não erro.
Catalogo le_catalogo(std::string_view corpo);

// ── E AGORA O QUE TOCA O MUNDO ──────────────────────────────────────────────

// busca_catalogo, pede a pagina de embutir e lê-a. Falso quando a rede não
// respondeu, ou quando a URL não tras identificador algum.
bool busca_catalogo(std::string_view crua, Catalogo* catalogo);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
