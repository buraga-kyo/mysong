// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA AQUISIÇÃO — src/nucleo/aquisicao.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As cinco funcções puras primeiro; o `fork` e o `exec` depois,
// e sozinhos no fim do arquivo, para que o olho veja de um relance quanto d'esta
// peça se prova e quanto não.
//
// DOMÍNIO ......... uma URL, o que o operador disse, e a raiz do acervo.
// CONTRA-DOMÍNIO .. um arquivo no logar certo, com etiqueta certa, e um Desfecho.
// INVARIANTE ...... o `exec` recebe VECTOR de argumentos, e nunca uma linha de
//                   shell: URL vinda do operador não passa por interpretador
//                   algum, donde não ha aspa nem ponto e virgula que faça o que
//                   não se pediu.
// Q.E.D. .......... não havendo shell, a injecção não é «improvavel»: é
//                   inexprimivel.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/aquisicao.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

namespace mysong::nucleo {

namespace {

// O comprimento maximo de UM componente de caminho, em octetos. Duzentos e
// quarenta, e não duzentos e cincoenta e cinco: sobram quinze para a extensão e
// para o «NN - » que o numero põe á frente.
constexpr std::size_t kMaxComponente = 240;

// apara — tira os espaços das duas pontas. Nome com espaço á frente existe no
// systema de arquivos e é fonte de confusão sem fim.
std::string apara(std::string_view crua) {
  std::size_t principio = 0, fim = crua.size();
  while (principio < fim && std::isspace(static_cast<unsigned char>(crua[principio])))
    ++principio;
  while (fim > principio && std::isspace(static_cast<unsigned char>(crua[fim - 1])))
    --fim;
  return std::string(crua.substr(principio, fim - principio));
}

}  // namespace

std::string saneia_nome(std::string_view crua) {
  std::string limpo;
  limpo.reserve(crua.size());
  for (const char letra : crua) {
    // A BARRA e o NUL são os dous unicos octetos que o kernel proscreve n'um
    // componente de caminho. Trocam-se, e não se apagam: apagar collaria
    // «AC/DC» em «ACDC», e o operador não reconheceria o que pediu.
    if (letra == '/') { limpo += '-'; continue; }
    if (letra == '\0') continue;
    limpo += letra;
  }
  limpo = apara(limpo);
  // Ponto inicial faz arquivo occulto, e faixa que se esconde do operador é
  // faixa perdida. `..` seria pior: subiria um degrau na hierarchia.
  while (!limpo.empty() && limpo.front() == '.') limpo.erase(limpo.begin());
  limpo = apara(limpo);
  if (limpo.size() > kMaxComponente) {
    limpo.resize(kMaxComponente);
    // Cortar por octeto pode partir um caracter UTF-8 pelo meio. Recúa-se até ao
    // byte lider, que é melhor que gravar um nome com meio caracter dentro.
    while (!limpo.empty() &&
           (static_cast<unsigned char>(limpo.back()) & 0xC0) == 0x80)
      limpo.pop_back();
    if (!limpo.empty()) limpo.pop_back();
    limpo = apara(limpo);
  }
  return limpo.empty() ? std::string("sem titulo") : limpo;
}

Pedido resolve(const Pedido& pedido, const EtiquetaRemota& remota) {
  Pedido feito = pedido;
  // O OPERADOR GANHA sempre. Cada campo cede á rede sómente quando o operador
  // calou, e nunca ao contrario: o contrario seria a rede a corrigir o operador.
  if (feito.titulo.empty()) feito.titulo = remota.titulo;
  if (feito.album.empty()) feito.album = remota.album;
  if (feito.numero == 0) feito.numero = remota.numero;
  if (feito.artista.empty()) {
    // A ordem: o que a rede chama artista, depois o canal, depois o desconhecido.
    // O canal entra em ULTIMO logar de proposito: elle é o que estava errado no
    // acervo de verdade, e sómente serve por não haver melhor.
    feito.artista = !remota.artista.empty() ? remota.artista : remota.canal;
  }
  if (feito.artista.empty()) feito.artista = "Desconhecido";
  if (feito.titulo.empty()) feito.titulo = "sem titulo";
  return feito;
}

std::filesystem::path destino(const std::filesystem::path& raiz,
                              const Pedido& pedido) {
  std::filesystem::path caminho = raiz / saneia_nome(pedido.artista);
  if (!pedido.album.empty()) caminho /= saneia_nome(pedido.album);
  std::string folha;
  if (pedido.numero > 0) {
    char molde[8] = {0};
    std::snprintf(molde, sizeof molde, "%02d - ", pedido.numero);
    folha = molde;
  }
  return caminho / (folha + saneia_nome(pedido.titulo));
}

std::vector<std::string> argumentos_da_sonda(const std::string& url) {
  // A ordem d'estes seis `--print` é o CONTRACTO com le_etiqueta_remota, e por
  // isso os dous vivem no mesmo arquivo e a prova afere os dous juntos.
  return {"yt-dlp",   "--no-warnings",      "--no-playlist",
          "--print",  "%(title)s",          "--print",
          "%(uploader)s", "--print",        "%(artist)s",
          "--print", "%(album)s",           "--print",
          "%(track_number)s", "--print",    "%(duration)s",
          "--",      url};
}

std::vector<std::string> argumentos_do_download(
    const std::string& url, const std::filesystem::path& molde) {
  return {"yt-dlp",
          "--no-warnings",
          "--no-playlist",
          // `--no-overwrites` é a segunda guarda contra perder arquivo. A
          // primeira é a checagem do destino; ter as duas quer dizer que uma
          // corrida entre duas aquisições não apaga o que a outra gravou.
          "--no-overwrites",
          "--extract-audio",
          "--audio-format", "mp3",
          "--audio-quality", "0",
          // Etiqueta NENHUMA se embute: quem a escreve é esta Casa, com a taglib,
          // e a razão está no tractado do cabeçalho.
          "--no-embed-metadata",
          "--output", molde.string() + ".%(ext)s",
          "--", url};
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
