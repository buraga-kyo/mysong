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

#include <sys/wait.h>
#include <unistd.h>

#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
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
    // Cortar por octeto parte caracter UTF-8 pelo meio. Anda-se pois PARA DEANTE
    // guardando a ultima FRONTEIRA que cabe, em vez de cortar e recuar: recuando,
    // um corte que já cahia em fronteira perdia um caracter á toa, e foi
    // exactamente esse o defeito que a prova apanhou nesta linha.
    std::size_t fronteira = 0;
    for (std::size_t i = 0; i <= limpo.size(); ++i) {
      const bool limite =
          i == limpo.size() ||
          (static_cast<unsigned char>(limpo[i]) & 0xC0) != 0x80;
      if (!limite) continue;
      if (i > kMaxComponente) break;
      fronteira = i;
    }
    limpo.resize(fronteira);
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

EtiquetaRemota le_etiqueta_remota(const std::string& sahida) {
  // Seis linhas, na ordem que argumentos_da_sonda fixou. Linha «NA» ou vazia é
  // campo que a rede não soube dizer: o yt-dlp imprime «NA» para o que falta, e
  // tomar esse «NA» por titulo poria uma faixa chamada NA no acervo.
  std::vector<std::string> linhas;
  std::istringstream fonte(sahida);
  std::string linha;
  while (std::getline(fonte, linha)) {
    if (!linha.empty() && linha.back() == '\r') linha.pop_back();
    linhas.push_back(linha == "NA" ? std::string() : apara(linha));
  }
  linhas.resize(6);  // faltando linha, ella fica vazia, e não lixo da anterior

  EtiquetaRemota remota;
  remota.titulo = linhas[0];
  remota.canal = linhas[1];
  remota.artista = linhas[2];
  remota.album = linhas[3];
  // Numero e duração vêm em texto. Texto que não é numero dá ZERO, e não lança:
  // a rede é fonte alheia, e fonte alheia manda lixo.
  const auto inteiro = [](const std::string& crua) {
    if (crua.empty()) return 0;
    for (const unsigned char c : crua)
      if (std::isdigit(c) == 0) return 0;
    return std::atoi(crua.c_str());
  };
  remota.numero = inteiro(linhas[4]);
  remota.duracao = inteiro(linhas[5]);
  return remota;
}

int corre(const std::vector<std::string>& argumentos, std::string* colhido) {
  if (argumentos.empty()) return -1;
  int cano[2] = {-1, -1};
  if (::pipe(cano) != 0) return -1;

  const ::pid_t filho = ::fork();
  if (filho < 0) { ::close(cano[0]); ::close(cano[1]); return -1; }
  if (filho == 0) {
    ::close(cano[0]);
    ::dup2(cano[1], STDOUT_FILENO);
    ::dup2(cano[1], STDERR_FILENO);
    ::close(cano[1]);
    // O vector vira argv aqui, no filho, e sem shell: `execvp` recebe os
    // argumentos tal e qual, donde a URL não atravessa interpretador algum.
    std::vector<char*> argv;
    argv.reserve(argumentos.size() + 1);
    for (const std::string& um : argumentos)
      argv.push_back(const_cast<char*>(um.c_str()));
    argv.push_back(nullptr);
    ::execvp(argv[0], argv.data());
    ::_exit(127);  // o 127 do shell para «commando não achado»
  }

  ::close(cano[1]);
  char pedaco[4096];
  ::ssize_t lidos = 0;
  while ((lidos = ::read(cano[0], pedaco, sizeof pedaco)) > 0)
    if (colhido != nullptr) colhido->append(pedaco, static_cast<std::size_t>(lidos));
  ::close(cano[0]);

  int estado = 0;
  if (::waitpid(filho, &estado, 0) < 0) return -1;
  return WIFEXITED(estado) ? WEXITSTATUS(estado) : -1;
}

bool sonda_url(const std::string& url, EtiquetaRemota* remota) {
  std::string colhido;
  if (corre(argumentos_da_sonda(url), &colhido) != 0) return false;
  if (remota != nullptr) *remota = le_etiqueta_remota(colhido);
  return true;
}

namespace {

// escreve_etiqueta — a etiqueta que esta Casa manda, e não a que a rede daria.
// UTF8 EXPLICITO: `TagLib::String` construida de std::string assume LATIN-1, e
// gravar «Máquina» assim fá-lo voltar «MÃ¡quina». Foi medido na issue #34.
bool escreve_etiqueta(const std::filesystem::path& arquivo,
                      const Pedido& pedido) {
  TagLib::FileRef punho(arquivo.c_str());
  if (punho.isNull() || punho.tag() == nullptr) return false;
  const auto utf8 = TagLib::String::UTF8;
  TagLib::Tag* etiqueta = punho.tag();
  etiqueta->setArtist(TagLib::String(pedido.artista, utf8));
  etiqueta->setTitle(TagLib::String(pedido.titulo, utf8));
  if (!pedido.album.empty())
    etiqueta->setAlbum(TagLib::String(pedido.album, utf8));
  if (pedido.numero > 0)
    etiqueta->setTrack(static_cast<unsigned>(pedido.numero));
  return punho.save();
}

// acha_o_que_ficou — o yt-dlp põe a extensão, e nós não a sabemos de antemão.
// Procura-se o irmão que principie pelo molde. Vazio quer dizer que nada ficou.
std::filesystem::path acha_o_que_ficou(const std::filesystem::path& molde) {
  std::error_code erro;
  const std::string folha = molde.filename().string();
  for (const auto& entrada :
       std::filesystem::directory_iterator(molde.parent_path(), erro)) {
    if (erro) break;
    if (!entrada.is_regular_file()) continue;
    const std::string nome = entrada.path().filename().string();
    if (nome.size() > folha.size() && nome.compare(0, folha.size(), folha) == 0)
      return entrada.path();
  }
  return {};
}

}  // namespace

Colheita baixa(const std::filesystem::path& raiz, const Pedido& pedido,
               std::filesystem::path* gravado) {
  EtiquetaRemota remota;
  if (!sonda_url(pedido.url, &remota)) {
    // Não se distingue aqui «yt-dlp ausente» de «URL recusada» pelo codigo, que
    // o `corre` devolve o mesmo menos um nos dous. Pergunta-se pois ao caminho.
    std::string nada;
    return corre({"yt-dlp", "--version"}, &nada) == 0 ? Colheita::UrlRecusada
                                                      : Colheita::SemFerramenta;
  }

  const Pedido feito = resolve(pedido, remota);
  const std::filesystem::path molde = destino(raiz, feito);
  std::error_code erro;
  std::filesystem::create_directories(molde.parent_path(), erro);
  if (erro) return Colheita::FalhouAoBaixar;

  // A PRIMEIRA guarda contra perder arquivo: havendo já irmão com este molde,
  // nada se corre. A segunda é o `--no-overwrites` na lista de argumentos.
  if (!acha_o_que_ficou(molde).empty()) {
    if (gravado != nullptr) *gravado = acha_o_que_ficou(molde);
    return Colheita::JaExiste;
  }

  std::string colhido;
  if (corre(argumentos_do_download(pedido.url, molde), &colhido) != 0)
    return Colheita::FalhouAoBaixar;

  const std::filesystem::path ficou = acha_o_que_ficou(molde);
  if (ficou.empty()) return Colheita::FalhouAoBaixar;
  if (gravado != nullptr) *gravado = ficou;
  return escreve_etiqueta(ficou, feito) ? Colheita::Colhido
                                        : Colheita::FalhouAEtiqueta;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
