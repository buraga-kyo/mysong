// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA AQUISIÇÃO, src/nucleo/aquisicao.cpp
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

#include "nucleo/letra.hpp"

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace mysong::nucleo {

namespace {

// O comprimento maximo de UM componente de caminho, em octetos. Duzentos e
// quarenta, e não duzentos e cincoenta e cinco: sobram quinze para a extensão e
// para o «NN - » que o numero põe á frente.
constexpr std::size_t kMaxComponente = 240;

// apara, tira os espaços das duas pontas. Nome com espaço á frente existe no
// systema de arquivos e é fonte de confusão sem fim.
std::string apara(std::string_view crua) {
  std::size_t principio = 0, fim = crua.size();
  while (principio < fim && std::isspace(static_cast<unsigned char>(crua[principio])))
    ++principio;
  while (fim > principio && std::isspace(static_cast<unsigned char>(crua[fim - 1])))
    --fim;
  return std::string(crua.substr(principio, fim - principio));
}

// O TECTO do inteiro que vem da rede. Um dia em segundos: dos tres campos que
// passam por aqui (numero de faixa, duração e anno), o maior legitimo é a
// duração, e um dia é o tecto que esta Casa já lhe dá no casamento do catalogo.
constexpr long kTectoDoInteiro = 86400;

// inteiro_da_rede, o inteiro que uma linha de fonte alheia diz. Texto que não
// seja digito de ponta a ponta dá ZERO, e não lança: a rede manda lixo, e zero
// é como esta Casa diz «não se soube». Lê-se por strtol, e não pela leitura
// antiga, que era comportamento INDEFINIDO em transbordo e vinte digitos vindos
// da rede transbordam. O que passa do tecto vae a zero pela mesma razão: aparar
// no proprio tecto poria por anno o numero do tecto, e isso seria mentira nova.
int inteiro_da_rede(const std::string& crua) {
  if (crua.empty()) return 0;
  for (const unsigned char c : crua)
    if (std::isdigit(c) == 0) return 0;
  errno = 0;
  const long lido = std::strtol(crua.c_str(), nullptr, 10);
  if (errno != 0 || lido > kTectoDoInteiro) return 0;
  return static_cast<int>(lido);
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

Pedido enriquece(const Pedido& pedido, const FichaMB& ficha) {
  // Aqui a ficha GANHA do que ha, ao contrario do resolve, em que o operador
  // ganha da rede. Não é contradicção: album e numero de hoje não são dictos do
  // operador, são o nome da lista e a posição n'ella, remendos confessados da
  // issue #13; e a ficha só existe quando a GRAVAÇÃO casou, donde o canonico é
  // mais verdade que o remendo. Artista e titulo ficam: são o que elle vê.
  Pedido feito = pedido;
  if (!ficha.album.empty()) feito.album = ficha.album;
  if (ficha.ano > 0) feito.ano = ficha.ano;
  if (ficha.numero > 0) feito.numero = ficha.numero;
  return feito;
}

std::filesystem::path destino(const std::filesystem::path& raiz,
                              const Pedido& pedido) {
  std::filesystem::path caminho = raiz / saneia_nome(pedido.artista);
  if (!pedido.album.empty()) caminho /= saneia_nome(pedido.album);
  std::string folha;
  if (pedido.numero > 0) {
    // O numero apara-se em quatro digitos, e o buffer é folgado. Sem a aparadura,
    // um numero absurdo vindo da rede (o `track_number` é campo alheio) daria
    // nome de faixa com dez digitos á frente; e com buffer justo, o compilador
    // accusa truncamento em Release, que foi como este defeito appareceu.
    const int cingido = pedido.numero > 9999 ? 9999 : pedido.numero;
    char molde[24] = {0};
    std::snprintf(molde, sizeof molde, "%02d - ", cingido);
    folha = molde;
  }
  return caminho / (folha + saneia_nome(pedido.titulo));
}

std::vector<std::string> bandeiras_do_motor(bool com_cookie) {
  // `--js-runtimes` porque o yt-dlp habilita o `deno` e mais nada por conta
  // propria; `--remote-components` porque a biblioteca que resolve o desafio elle
  // a busca, e sem permissão não a busca.
  std::vector<std::string> bandeiras{"--js-runtimes", kMotorDeJs,
                                     "--remote-components",
                                     kComponenteDoDesafio};
  // O cookie entra SÓ quando se pede, e a razão está no cabeçalho: medido, elle
  // faz o yt-dlp recusar toda URL. Quem o ligar sabe por que o liga.
  if (com_cookie) {
    bandeiras.emplace_back("--cookies-from-browser");
    bandeiras.emplace_back(kNavegadorDoCookie);
  }
  return bandeiras;
}

std::vector<std::string> argumentos_da_sonda(const std::string& url,
                                             bool com_cookie) {
  // A ordem d'estes seis `--print` é o CONTRACTO com le_etiqueta_remota, e por
  // isso os dous vivem no mesmo arquivo e a prova afere os dous juntos.
  std::vector<std::string> ditos{"yt-dlp"};
  const std::vector<std::string> motor = bandeiras_do_motor(com_cookie);
  ditos.insert(ditos.end(), motor.begin(), motor.end());
  const std::vector<std::string> resto = {"--no-warnings",      "--no-playlist",
          "--print",  "%(title)s",          "--print",
          "%(uploader)s", "--print",        "%(artist)s",
          "--print", "%(album)s",           "--print",
          "%(track_number)s", "--print",    "%(duration)s",
          "--",      url};
  ditos.insert(ditos.end(), resto.begin(), resto.end());
  return ditos;
}

bool eh_playlist_url(const std::string& url) {
  return !id_da_playlist(url).empty() ||
         url.find("youtube.com/playlist?") != std::string::npos ||
         url.find("list=") != std::string::npos;
}

std::vector<std::string> argumentos_da_playlist(const std::string& url,
                                                bool com_cookie) {
  std::vector<std::string> ditos{"yt-dlp"};
  const std::vector<std::string> motor = bandeiras_do_motor(com_cookie);
  ditos.insert(ditos.end(), motor.begin(), motor.end());
  const std::vector<std::string> resto = {
      "--no-warnings", "--flat-playlist", "--skip-download",
      "--print", "%(webpage_url)s", "--", url};
  ditos.insert(ditos.end(), resto.begin(), resto.end());
  return ditos;
}

std::vector<std::string> le_urls_da_playlist(const std::string& sahida) {
  std::vector<std::string> urls;
  std::istringstream linhas(sahida);
  std::string url;
  while (std::getline(linhas, url)) {
    const std::string aparada = apara(url);
    if (!aparada.empty() && aparada != "NA") urls.push_back(aparada);
  }
  return urls;
}

bool busca_playlist_na_rede(const std::string& url,
                            std::vector<std::string>* urls) {
  if (urls == nullptr || url.empty() || !eh_playlist_url(url) ||
      !id_da_playlist(url).empty())
    return false;
  std::string colhido;
  if (corre(argumentos_da_playlist(url), &colhido) != 0) return false;
  *urls = le_urls_da_playlist(colhido);
  return true;
}

std::vector<std::string> argumentos_do_download(
    const std::string& url, const std::filesystem::path& molde,
    bool com_cookie) {
  std::vector<std::string> ditos{"yt-dlp"};
  const std::vector<std::string> motor = bandeiras_do_motor(com_cookie);
  ditos.insert(ditos.end(), motor.begin(), motor.end());
  const std::vector<std::string> resto = {
          "--no-warnings",
          "--no-playlist",
          "--newline",
          // `--no-overwrites` é a segunda guarda contra perder arquivo. A
          // primeira é a checagem do destino; ter as duas quer dizer que uma
          // corrida entre duas aquisições não apaga o que a outra gravou.
          "--no-overwrites",
          "--extract-audio",
          "--audio-format", "mp3",
          "--audio-quality", "0",
          // Etiqueta de TEXTO nenhuma se embute: quem a escreve é esta Casa, com
          // a taglib, e a razão está no tractado do cabeçalho.
          "--no-embed-metadata",
          // A MINIATURA, essa pede-se, e vae para dentro da etiqueta (issue #81).
          // Tres cousas medidas antes de escolher, e todas sobre uma URL de verdade:
          //
          //  1. O YouTube dá a miniatura em WEBP («Writing video thumbnail 37 to:
          //     faixa.webp»). Nem toda build do chafa lê WebP, e leitor de etiqueta
          //     alheio lê-o ainda peor dentro de um APIC. D'onde o
          //     `--convert-thumbnails jpg`, que corre por ffmpeg; e ffmpeg já é
          //     dependencia dura do `--extract-audio` acima, donde nada de novo entra.
          //  2. EMBUTIR ganha de gravar ao lado. Com `--write-thumbnail` a capa pousa
          //     com o nome do MOLDE, e ficou «01 - Prelude.jpg»; esse nome não está
          //     entre os doze que a `nomes_de_capa()` procura, donde a capa ficaria no
          //     disco e o painel continuaria vazio. Embutida, é o que a `capa.cpp` já
          //     sabe ler, e sobrevive a mover a faixa.
          //  3. A ordem não morde. A etiqueta escreve-se DEPOIS, no `escreve_etiqueta`,
          //     e o `save()` da taglib PRESERVA o quadro: 23689 octetos antes, 23689
          //     depois. Fosse elle deitar a arte fóra, embutir seria escolha morta.
          //
          // Video sem miniatura alguma não faz a baixa falhar: o yt-dlp diz que não ha
          // o que embutir e segue (postprocessor/embedthumbnail.py).
          "--embed-thumbnail",
          "--convert-thumbnails", "jpg",
          "--output", molde.string() + ".%(ext)s",
          "--", url};
  ditos.insert(ditos.end(), resto.begin(), resto.end());
  return ditos;
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
  remota.numero = inteiro_da_rede(linhas[4]);
  remota.duracao = inteiro_da_rede(linhas[5]);
  return remota;
}

std::string_view razao_da_colheita(Colheita colheita) {
  switch (colheita) {
    case Colheita::Colhido: return "baixado";
    case Colheita::ColhidoDuvidoso:
      return "baixado por titulo, sem a gravação: confira";
    case Colheita::SemFerramenta: return "falta o yt-dlp: uv tool install yt-dlp";
    case Colheita::UrlRecusada: return "o yt-dlp não leu essa URL";
    case Colheita::JaExiste: return "essa faixa já está no acervo";
    case Colheita::FalhouAoBaixar: return "o download falhou";
    case Colheita::FalhouAEtiqueta: return "baixou, mas a etiqueta não se escreveu";
    case Colheita::Duvidosa: return "achado algum casou: fica duvidosa";
  }
  return "desfecho sem nome";
}

std::string_view nome_da_fonte(Fonte fonte) {
  switch (fonte) {
    case Fonte::YouTube: return "YouTube";
    case Fonte::YouTubeMusic: return "YouTube Music";
    case Fonte::Spotify: return "Spotify";
  }
  return "fonte sem nome";
}

std::optional<int> progresso_do_yt_dlp(std::string_view linha) {
  const std::string_view marca = "[download]";
  if (linha.substr(0, marca.size()) != marca) return std::nullopt;
  std::size_t i = marca.size();
  while (i < linha.size() && linha[i] == ' ') ++i;
  if (i == linha.size() || linha[i] < '0' || linha[i] > '9')
    return std::nullopt;
  int inteiro = 0;
  while (i < linha.size() && linha[i] >= '0' && linha[i] <= '9') {
    inteiro = inteiro * 10 + linha[i++] - '0';
    if (inteiro > 100) return std::nullopt;
  }
  if (i < linha.size() && linha[i] == '.') {
    ++i;
    if (i == linha.size() || linha[i] < '0' || linha[i] > '9')
      return std::nullopt;
    while (i < linha.size() && linha[i] >= '0' && linha[i] <= '9') ++i;
  }
  return i < linha.size() && linha[i] == '%' ? std::optional<int>(inteiro)
                                             : std::nullopt;
}

Fonte proxima_fonte(Fonte fonte) {
  switch (fonte) {
    case Fonte::YouTube: return Fonte::YouTubeMusic;
    case Fonte::YouTubeMusic: return Fonte::Spotify;
    case Fonte::Spotify: return Fonte::YouTube;
  }
  return Fonte::YouTube;
}

int corre(const std::vector<std::string>& argumentos, std::string* colhido,
         const std::function<void(std::string_view)>& linha, bool unir_erros) {
  if (argumentos.empty()) return -1;
  int cano[2] = {-1, -1};
  if (::pipe(cano) != 0) return -1;

  const ::pid_t filho = ::fork();
  if (filho < 0) { ::close(cano[0]); ::close(cano[1]); return -1; }
  if (filho == 0) {
    ::close(cano[0]);
    ::dup2(cano[1], STDOUT_FILENO);
    // O STDERR NÃO se junta ao stdout. Medido nesta Casa: o yt-dlp escreve no
    // stderr o aviso «Deprecated Feature: Support for Python version 3.10 has
    // been deprecated», e juntando-se os dous esse aviso vinha como PRIMEIRA
    // linha, donde o titulo da faixa passava a ser o aviso e o canal passava a
    // ser o titulo. O contracto dos `--print` é do stdout, e sómente d'elle.
    //
    // E o stderr vae para o buraco, e não para o terminal: esta Casa corre debaixo
    // de uma tela do FTXUI, e uma linha de aviso no meio do quadro estraga-o.
    if (unir_erros) {
      ::dup2(cano[1], STDERR_FILENO);
    } else {
      const int buraco = ::open("/dev/null", O_WRONLY);
      if (buraco >= 0) { ::dup2(buraco, STDERR_FILENO); ::close(buraco); }
    }
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
  std::string pendente;
  while ((lidos = ::read(cano[0], pedaco, sizeof pedaco)) > 0) {
    if (colhido != nullptr) colhido->append(pedaco, static_cast<std::size_t>(lidos));
    if (!linha) continue;
    for (ssize_t i = 0; i < lidos; ++i) {
      if (pedaco[i] == '\n' || pedaco[i] == '\r') {
        if (!pendente.empty()) linha(pendente);
        pendente.clear();
      } else if (pendente.size() < 4096) {
        pendente += pedaco[i];
      }
    }
  }
  if (linha && !pendente.empty()) linha(pendente);
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

// escreve_etiqueta, a etiqueta que esta Casa manda, e não a que a rede daria.
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
  // O ano entra quando a fonte o deu (issue #56) ou quando a gravação casou no
  // MusicBrainz (issue #57); zero é «não se soube», e gravá-lo seria mentira.
  if (pedido.ano > 0) etiqueta->setYear(static_cast<unsigned>(pedido.ano));
  return punho.save();
}

// acha_o_que_ficou, o yt-dlp põe a extensão, e nós não a sabemos de antemão.
// Procura-se o irmão que principie pelo molde. Vazio quer dizer que nada ficou.
// minuscula_ascii, a cadeia em caixa baixa, para as letras da taboa de ASCII. Não
// dobra acento, e é de proposito: dobrar acento em UTF-8 pede taboa que esta Casa
// não tem, e prometter menos é melhor que prometter e falhar no «á» contra o «a».
std::string minuscula_ascii(std::string_view crua) {
  std::string baixa;
  baixa.reserve(crua.size());
  for (const unsigned char letra : crua)
    baixa += static_cast<char>(letra >= 'A' && letra <= 'Z' ? letra + 32 : letra);
  return baixa;
}

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

int melhor_achado(const std::vector<Achado>& achados, const Pedido& pedido,
                  int tolerancia) {
  // Pedido sem duração não casa. Sem ella não ha crivo algum, e a tarefa manda
  // marcar por duvidosa em vez de baixar cousa errada calada.
  if (pedido.duracao <= 0) return -1;
  const std::string alvo = minuscula_ascii(pedido.titulo);
  int eleito = -1, eleito_com_titulo = -1;
  int distancia_do_eleito = 0, distancia_com_titulo = 0;
  for (std::size_t i = 0; i < achados.size(); ++i) {
    const Achado& achado = achados[i];
    if (achado.duracao <= 0) continue;  // achado sem duração não se pode crivar
    const int distancia = achado.duracao > pedido.duracao
                              ? achado.duracao - pedido.duracao
                              : pedido.duracao - achado.duracao;
    // O CRIVO da duração, e é o UNICO logar onde elle se applica. Não se repete no
    // valor inicial dos melhores, e é de proposito: repetido, tirar este `continue`
    // não mudava nada, e a mutação que o tirasse sobreviveria á bateria. Uma guarda
    // que ninguem pode matar é guarda que ninguem sabe se presta.
    if (distancia > tolerancia) continue;
    if (eleito < 0 || distancia < distancia_do_eleito) {
      distancia_do_eleito = distancia;
      eleito = static_cast<int>(i);
    }
    // O segundo crivo: o titulo do pedido dentro do titulo do achado. Entre os que
    // passam a duração, este separa a faixa certa da vizinha de egual comprimento.
    if (alvo.empty() ||
        minuscula_ascii(achado.titulo).find(alvo) == std::string::npos)
      continue;
    if (eleito_com_titulo < 0 || distancia < distancia_com_titulo) {
      distancia_com_titulo = distancia;
      eleito_com_titulo = static_cast<int>(i);
    }
  }
  return eleito_com_titulo >= 0 ? eleito_com_titulo : eleito;
}

int achado_mais_proximo(const std::vector<Achado>& achados, int duracao) {
  // O eleitor do caminho ISRC, e o contrario declarado do melhor_achado: o
  // termo que trouxe estes achados é o ISRC, que nomeia a gravação, donde o
  // TITULO não entra e distancia alguma exclue. A duração exacta desempata, e
  // não criva, por ordem do usuario (RULINGS R3): cover de titulo egual perde
  // aqui para a art track de titulo estranho, que é o Aceite da issue.
  if (achados.empty()) return -1;
  if (duracao <= 0) return 0;  // sem alvo não ha desempate: vale o primeiro
  int eleito = -1, do_eleito = 0;
  for (std::size_t i = 0; i < achados.size(); ++i) {
    if (achados[i].duracao <= 0) continue;  // «não disse» não desempata
    const int longe = achados[i].duracao > duracao
                          ? achados[i].duracao - duracao
                          : duracao - achados[i].duracao;
    if (eleito < 0 || longe < do_eleito) {
      do_eleito = longe;
      eleito = static_cast<int>(i);
    }
  }
  return eleito < 0 ? 0 : eleito;  // nenhum disse duração: vale o primeiro
}

Pedido encommenda_do_achado(const Achado& achado) {
  Pedido pedido;
  pedido.url = achado.url;
  pedido.artista = achado.artista;
  pedido.album = achado.album;
  pedido.titulo = achado.faixa;
  pedido.numero = achado.numero;
  pedido.ano = achado.ano;
  pedido.fonte = achado.fonte;
  pedido.id_spotify = achado.id_spotify;  // o link do MusicBrainz (issue #57)
  // A duração sómente no pedido sem URL, onde ella criva o casamento. Com URL,
  // pô-la mudaria o pedido de hoje sem lhe mudar o desfecho.
  if (achado.url.empty()) pedido.duracao = achado.duracao;
  return pedido;
}

std::vector<Achado> achados_do_catalogo(const Catalogo& catalogo,
                                        const std::string& termo) {
  std::vector<Achado> achados;
  const std::string alvo = minuscula_ascii(termo);
  for (const FaixaDoCatalogo& faixa : catalogo.faixas) {
    // Titulo OU artista, como o filtro da secção Lista: com fonte de musica o
    // que se busca é tanto um como o outro.
    if (minuscula_ascii(faixa.titulo).find(alvo) == std::string::npos &&
        minuscula_ascii(faixa.artista).find(alvo) == std::string::npos)
      continue;
    Achado achado;
    achado.titulo = faixa.titulo;
    achado.faixa = faixa.titulo;  // no catalogo o canonico é o proprio titulo
    achado.artista = faixa.artista;
    achado.album = catalogo.nome;  // o album é o nome da lista (issue #13)
    achado.numero = faixa.numero;
    // Milesimos a segundos, ao mais proximo: truncar perderia meio segundo por
    // faixa, e a tolerancia do casamento conta-os.
    achado.duracao = (faixa.duracao_ms + 500) / 1000;
    achado.fonte = Fonte::Spotify;
    achado.id_spotify = faixa.id_do_track;  // por onde o MusicBrainz acha a gravação
    achados.push_back(achado);
  }
  return achados;
}

bool busca_na_rede(const std::string& termo, Fonte fonte, int quantos,
                   std::vector<Achado>* achados) {
  if (termo.empty()) return false;
  std::string colhido;
  if (corre(argumentos_da_busca(termo, quantos, fonte), &colhido) != 0)
    return false;
  if (achados == nullptr) return true;
  *achados = le_achados(colhido);
  // A FONTE estampa-se na volta, e não em le_achados: elle lê linhas, e as linhas
  // não dizem de onde vieram.
  for (Achado& achado : *achados) achado.fonte = fonte;
  return true;
}

Colheita baixa(const std::filesystem::path& raiz, const Pedido& pedido,
               std::filesystem::path* gravado) {
  // SEM URL, mas com titulo: busca-se o audio por si (issue #13), agora pela
  // GRAVAÇÃO antes do titulo (issue #57). O MusicBrainz resolve a faixa n'uma
  // ficha; os termos de ISRC nomeiam a gravação exacta, e entre os achados a
  // duração exacta DESEMPATA sem excluir (RULINGS R3); o termo de hoje fica por
  // derradeiro, e o que baixar por elle sahe confessando a duvida no desfecho.
  if (pedido.url.empty()) {
    if (pedido.titulo.empty()) return Colheita::UrlRecusada;
    FichaMB ficha;
    // A duração do catalogo cinge-se a UM DIA antes de virar milesimos: vinda
    // da rede, um valor absurdo estouraria a conta por mil; cingida, vale «não
    // disse», que é o que um numero d'esses de facto diz.
    const int do_catalogo = pedido.duracao > 0 && pedido.duracao <= 86400
                                ? pedido.duracao * 1000
                                : 0;
    resolve_gravacao(pedido.id_spotify, pedido.artista, pedido.titulo,
                     do_catalogo, &ficha);
    // Não casando, a ficha fica vazia: o enriquecimento devolve o pedido tal e
    // qual e os termos reduzem-se ao de hoje, que é o caminho antigo inteiro.
    const Pedido rico = enriquece(pedido, ficha);
    const int alvo_ms = ficha.duracao_ms > 0 ? ficha.duracao_ms : do_catalogo;
    const std::vector<std::string> termos =
        termos_de_busca(ficha, pedido.artista, pedido.titulo);
    for (std::size_t i = 0; i < termos.size(); ++i) {
      const bool de_hoje = i + 1 == termos.size();  // o derradeiro é o de hoje
      std::vector<Achado> achados;
      // A FONTE de cada termo, e a differença é de segurança, não de gosto. O
      // termo de hoje vae na fonte do PROPRIO pedido, que é o invariante da
      // issue #56: quem pediu é quem sabe onde o audio d'elle se procura. Os
      // termos de ISRC vão no ytsearch, e SÓ n'elle, porque é a unica fonte em
      // que a propriedade de que este caminho vive foi MEDIDA: ISRC não
      // indexado devolve NADA. A busca do music.youtube.com é difusa e tende a
      // devolver ALGO; como o caminho do ISRC não criva por duração (RULINGS
      // R3, ordem do usuario), qualquer achado alheio seria eleito por
      // proximidade e a faixa sahiria por Colhido, casada com confiança e
      // errada. Pinar o ISRC aqui é o que torna essa via inexprimivel.
      const Fonte onde = de_hoje ? pedido.fonte : Fonte::YouTube;
      if (!busca_na_rede(termos[i], onde, 10, &achados))
        return Colheita::SemFerramenta;
      const int qual =
          de_hoje ? melhor_achado(achados, rico, TOLERANCIA_DO_CASAMENTO)
                  : achado_mais_proximo(achados, (alvo_ms + 500) / 1000);
      if (qual < 0) continue;  // busca vazia ou nada casou: o termo seguinte
      Pedido com_url = rico;
      com_url.url = achados[static_cast<std::size_t>(qual)].url;
      const Colheita fim = baixa(raiz, com_url, gravado);
      // Pelo termo de hoje o casamento é o da issue #13, que aceita cover e
      // versão ao vivo: o Colhido troca-se pelo desfecho que confessa isso.
      return de_hoje && fim == Colheita::Colhido ? Colheita::ColhidoDuvidoso
                                                 : fim;
    }
    return Colheita::Duvidosa;
  }

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

  if (pedido.noticia) pedido.noticia(std::nullopt, {});
  std::string erro_da_rede;
  const auto observa = [&pedido, &erro_da_rede](std::string_view linha) {
    if (const auto porcentagem = progresso_do_yt_dlp(linha)) {
      if (pedido.noticia) pedido.noticia(porcentagem, {});
    } else if (linha.substr(0, 6) == "ERROR:") {
      erro_da_rede.clear();
      for (const unsigned char letra : linha.substr(0, 180))
        if (letra >= 32 && letra != 127) erro_da_rede += static_cast<char>(letra);
    }
  };
  if (corre(argumentos_do_download(pedido.url, molde), nullptr, observa, true) != 0) {
  std::string colhido;
  if (corre(argumentos_do_download(pedido.url, molde), &colhido) != 0)
    return Colheita::FalhouAoBaixar;

  const std::filesystem::path ficou = acha_o_que_ficou(molde);
  if (ficou.empty()) return Colheita::FalhouAoBaixar;
  if (gravado != nullptr) *gravado = ficou;

  // A LETRA busca-se AQUI, no momento do download, e nunca ao escutar (issue #14).
  // Falhar não custa a faixa: a letra é ornamento, e o desfecho não muda por ella.
  // Corre DEPOIS de o audio estar no logar, para que uma rede lenta não atrase o
  // que o operador de facto pediu.
  Letra letra;
  if (busca_letra(feito.artista, feito.titulo, &letra)) grava_lrc(ficou, letra);

  return escreve_etiqueta(ficou, feito) ? Colheita::Colhido
                                        : Colheita::FalhouAEtiqueta;
}

std::string codifica_para_url(std::string_view crua) {
  static const char kHex[] = "0123456789ABCDEF";
  std::string feita;
  feita.reserve(crua.size());
  for (const unsigned char c : crua) {
    // A taboa dos LIVRES da RFC 3986, e nada mais: espaço, `#`, `&`, `+` e todo
    // UTF-8 sahem por cento, e não ha byte que atravesse por engano.
    const bool livre = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                       (c >= '0' && c <= '9') || c == '-' || c == '.' ||
                       c == '_' || c == '~';
    if (livre) {
      feita += static_cast<char>(c);
      continue;
    }
    feita += '%';
    feita += kHex[c >> 4];
    feita += kHex[c & 0xF];
  }
  return feita;
}

std::vector<std::string> argumentos_da_busca(const std::string& termo,
                                             int quantos, Fonte fonte,
                                             bool com_cookie) {
  // Apara-se em vinte: busca maior gasta rede e não cabe na tabella. E em um pelo
  // baixo, que buscar zero é pedido sem sentido. A MUSICA apara em dez, e AQUI,
  // para o tecto valer em todo caminho: o da tela e o da baixa sem URL.
  const int tecto = fonte == Fonte::YouTubeMusic ? ACHADOS_DA_MUSICA : 20;
  const int quantas = quantos < 1 ? 1 : (quantos > tecto ? tecto : quantos);
  std::vector<std::string> ditos{"yt-dlp"};
  const std::vector<std::string> motor = bandeiras_do_motor(com_cookie);
  ditos.insert(ditos.end(), motor.begin(), motor.end());
  ditos.emplace_back("--no-warnings");
  if (fonte == Fonte::YouTubeMusic) {
    // SEM o --flat-playlist, e é medido: com elle os campos da musica vêm NA.
    ditos.emplace_back("--playlist-items");
    ditos.emplace_back("1:" + std::to_string(quantas));
  } else {
    ditos.emplace_back("--flat-playlist");
  }
  // O alvo. O SPOTIFY busca no YouTube, e não é descuido: o catalogo é metadado,
  // o audio vem do YouTube, e a tela busca a fonte Spotify no catalogo local.
  const std::string alvo =
      fonte == Fonte::YouTubeMusic
          ? "https://music.youtube.com/search?q=" + codifica_para_url(termo) +
                "#songs"
          : "ytsearch" + std::to_string(quantas) + ":" + termo;
  const std::vector<std::string> resto = {
          "--print", "%(title)s",
          "--print", "%(uploader)s",
          "--print", "%(duration)s",
          "--print", "%(webpage_url)s",
          "--print", "%(artist)s",
          "--print", "%(album)s",
          "--print", "%(track)s",
          "--print", "%(release_year)s",
          "--",
          alvo};
  ditos.insert(ditos.end(), resto.begin(), resto.end());
  return ditos;
}

std::vector<Achado> le_achados(const std::string& sahida) {
  std::vector<std::string> linhas;
  std::istringstream fonte(sahida);
  std::string linha;
  while (std::getline(fonte, linha)) {
    if (!linha.empty() && linha.back() == '\r') linha.pop_back();
    linhas.push_back(linha == "NA" ? std::string() : apara(linha));
  }
  std::vector<Achado> achados;
  // Oito linhas por achado. Sobrando linhas que não completem um grupo de oito,
  // descartam-se: achado meio não se mostra, que o operador o escolheria e a baixa
  // falharia sem URL.
  for (std::size_t i = 0; i + 7 < linhas.size(); i += 8) {
    Achado achado;
    achado.titulo = linhas[i];
    achado.canal = linhas[i + 1];
    achado.duracao = inteiro_da_rede(linhas[i + 2]);
    achado.url = linhas[i + 3];
    achado.artista = linhas[i + 4];
    achado.album = linhas[i + 5];
    achado.faixa = linhas[i + 6];
    achado.ano = inteiro_da_rede(linhas[i + 7]);
    if (achado.url.empty()) continue;  // sem URL não ha o que baixar
    achados.push_back(achado);
  }
  return achados;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
