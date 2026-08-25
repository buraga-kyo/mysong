// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA VARREDURA — src/nucleo/varredura.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. A taglib mora aqui e sómente aqui, atras do punho oculto.
//
// DOMÍNIO ......... raízes de acervo em disco, e o índice antigo se houver.
// CONTRA-DOMÍNIO .. o índice novo, e o progresso.
// INVARIANTE ...... funcção alguma d'aqui deixa escapar excepção do
//                   std::filesystem: acervo é disco alheio, e disco alheio
//                   falha. Toda falha conta-se e a corrida segue.
// Q.E.D. .......... o passo é a unidade, e o estado inteiro vive no punho: donde
//                   se pode parar entre dous passos sem perder cousa alguma.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/varredura.hpp"

#include "nucleo/video.hpp"

#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

#include <algorithm>
#include <cstdint>
#include <cctype>
#include <set>
#include <system_error>
#include <utility>
#include <vector>

namespace mysong::nucleo {

namespace {

// As extensões que se offerecem á taglib, em minuscula e com o ponto. A lista é
// FECHADA de proposito: extensão nova é decisão, e não descuido.
constexpr std::string_view kExtensoes[] = {".mp3", ".flac", ".ogg",  ".oga",
                                           ".opus", ".m4a", ".mp4", ".wav",
                                           ".wma", ".aac", ".ape", ".wv"};

std::string minuscula(std::string_view crua) {
  std::string baixa;
  baixa.reserve(crua.size());
  for (const char letra : crua)
    baixa += static_cast<char>(
        std::tolower(static_cast<unsigned char>(letra)));
  return baixa;
}


// numero_e_titulo — parte `NN - Titulo` no numero e no titulo. Sem numero á
// frente, o titulo é o nome inteiro e o numero fica zero, que quer dizer «sem
// numero» e não «faixa zero». O separador aceita-se com ou sem espaços, e tanto
// hyphen como ponto: `01 - Tear`, `01-Tear` e `01. Tear` sahem eguaes.
void numero_e_titulo(const std::string& talo, int* numero, std::string* titulo) {
  *numero = 0;
  *titulo = talo;
  std::size_t i = 0;
  while (i < talo.size() && std::isdigit(static_cast<unsigned char>(talo[i])))
    ++i;
  if (i == 0 || i > 3) return;  // sem digitos á frente, ou numero improvavel

  std::size_t j = i;
  while (j < talo.size() && talo[j] == ' ') ++j;
  if (j < talo.size() && (talo[j] == '-' || talo[j] == '.')) ++j;
  else if (j == i) return;  // digitos collados a letra: é nome, e não numero
  while (j < talo.size() && talo[j] == ' ') ++j;
  if (j >= talo.size()) return;  // sómente o numero, sem titulo depois

  *numero = std::stoi(talo.substr(0, i));
  *titulo = talo.substr(j);
}

}  // namespace

Faixa deriva_do_caminho(const std::filesystem::path& caminho,
                        const std::filesystem::path& raiz) {
  Faixa faixa;
  faixa.caminho = caminho.string();
  faixa.raiz = raiz.string();
  // Os quatro bits acendem-se TODOS: a dedução é o piso, e quem lê a etiqueta
  // apaga o bit do que a etiqueta disser. Assim nenhum campo fica a dizer que
  // veio da etiqueta quando veio do caminho.
  faixa.deduzido = kDeduziuArtista | kDeduziuAlbum | kDeduziuTitulo |
                   kDeduziuNumero;

  numero_e_titulo(caminho.stem().string(), &faixa.numero, &faixa.titulo);

  // Os componentes ENTRE a raiz e o arquivo. Vazio quer dizer arquivo posto
  // directamente na raiz, e ahi artista e album ficam vazios de proposito.
  std::vector<std::string> degraus;
  const std::filesystem::path relativo =
      std::filesystem::relative(caminho.parent_path(), raiz);
  for (const std::filesystem::path& parte : relativo)
    if (parte != "." && parte != "..") degraus.push_back(parte.string());

  if (!degraus.empty()) {
    faixa.artista = degraus.front();  // o componente logo sob a raiz
    faixa.album = degraus.back();     // o directorio que contem o arquivo
    // Hierarchia de UM degrau: o mesmo directorio seria artista e album, e
    // dizer que o album se chama como o artista é affirmar o que não se sabe.
    if (degraus.size() == 1) faixa.album.clear();
  }
  return faixa;
}

// O PUNHO. Todo o estado da corrida vive aqui, e é por isso que se pode parar
// entre dous passos sem perder cousa alguma.
struct Varredura::Punho {
  std::filesystem::path banco;
  std::vector<std::filesystem::path> raizes;

  // A FASE. Lista-se raiz por raiz, depois lê-se arquivo por arquivo, e o
  // ultimo passo conclue. Fases separadas, e não entrelaçadas: a conta do
  // aceite (passos >= arquivos) sómente se lê se cada arquivo tiver o seu passo.
  enum class Fase { Listar, Ler, Concluir, Fim } fase = Fase::Listar;
  std::size_t raiz_corrente = 0;
  std::size_t arquivo_corrente = 0;

  // Os arquivos achados, com a raiz de que vieram. O canónico é a IDENTIDADE:
  // arquivo alcançavel por duas raízes que se sobrepõem entra UMA vez.
  std::vector<std::pair<std::filesystem::path, std::filesystem::path>> achados;
  std::set<std::string> vistos;

  Progresso progresso;
  Desfecho desfecho = Desfecho::NaoComecou;
  std::unique_ptr<Escriba> escriba;
  std::unique_ptr<Biblioteca> antigo;
};

// lista_raiz — colhe os arquivos de UMA raiz. Directorio symbólico NÃO se desce:
// é o que impede o laço de ligações de fazer a varredura girar sem fim, e conta
// em `ligacoes_saltadas` para que o operador saiba que ficou cousa por ver.
using Achado = std::pair<std::filesystem::path, std::filesystem::path>;

void lista_raiz(const std::filesystem::path& raiz, Progresso* progresso,
                std::set<std::string>* vistos, std::vector<Achado>* achados) {
  std::error_code erro;
  if (!std::filesystem::is_directory(raiz, erro) || erro) {
    ++progresso->raizes_falhadas;
    return;
  }
  auto opcoes = std::filesystem::directory_options::skip_permission_denied;
  std::filesystem::recursive_directory_iterator anda(raiz, opcoes, erro);
  if (erro) {
    ++progresso->raizes_falhadas;
    return;
  }
  const std::filesystem::recursive_directory_iterator fim;
  for (; anda != fim; anda.increment(erro)) {
    if (erro) { erro.clear(); continue; }  // entrada illegivel não para a raiz
    const std::filesystem::directory_entry& entrada = *anda;
    if (entrada.is_symlink(erro) && entrada.is_directory(erro)) {
      ++progresso->ligacoes_saltadas;
      anda.disable_recursion_pending();
      continue;
    }
    if (!entrada.is_regular_file(erro) || erro) { erro.clear(); continue; }
    if (!extensao_que_interessa(entrada.path().extension().string())) {
      ++progresso->recusadas;
      continue;
    }
    const std::string canonico =
        std::filesystem::weakly_canonical(entrada.path(), erro).string();
    if (erro) { erro.clear(); continue; }
    if (!vistos->insert(canonico).second) continue;  // já veio por outra
    achados->emplace_back(entrada.path(), raiz);
  }
}

// le_etiqueta — sobrepõe á faixa derivada o que a etiqueta disser, e APAGA o bit
// da máscara de cada campo que ella disse. Falso quando taglib não abre o
// arquivo: é o `.txt` renomeado para `.mp3`, e esse não entra no índice.
bool le_etiqueta(const std::filesystem::path& caminho, Faixa* faixa) {
  TagLib::FileRef arquivo(caminho.c_str());
  if (arquivo.isNull() || arquivo.audioProperties() == nullptr) return false;

  // A TAXA DE AMOSTRAGEM é o discriminante, e não `isValid()`. Medido nesta Casa
  // sobre um `.txt` renomeado para `.mp3`: `isNull` dá falso, `isValid` dá
  // VERDADEIRO, e sómente a taxa e a duração sahem em zero. A taglib aceita
  // abrir o que não é audio e dá-lhe propriedades vazias; audio de verdade nunca
  // tem taxa zero, e é por ella que se recusa.
  if (arquivo.audioProperties()->sampleRate() <= 0) return false;

  if (const TagLib::Tag* etiqueta = arquivo.tag()) {
    const std::string artista = etiqueta->artist().to8Bit(true);
    const std::string album = etiqueta->album().to8Bit(true);
    const std::string titulo = etiqueta->title().to8Bit(true);
    if (!artista.empty()) {
      faixa->artista = saneia_utf8(artista);
      faixa->deduzido &= ~static_cast<unsigned>(kDeduziuArtista);
    }
    if (!album.empty()) {
      faixa->album = saneia_utf8(album);
      faixa->deduzido &= ~static_cast<unsigned>(kDeduziuAlbum);
    }
    if (!titulo.empty()) {
      faixa->titulo = saneia_utf8(titulo);
      faixa->deduzido &= ~static_cast<unsigned>(kDeduziuTitulo);
    }
    if (etiqueta->track() != 0) {
      faixa->numero = static_cast<int>(etiqueta->track());
      faixa->deduzido &= ~static_cast<unsigned>(kDeduziuNumero);
    }
    faixa->anno = static_cast<int>(etiqueta->year());
  }
  faixa->duracao = arquivo.audioProperties()->lengthInSeconds();
  return true;
}

// trata_arquivo — um arquivo, um passo. Aqui vive o INCREMENTAL: hora e tamanho
// eguaes aos do índice antigo copiam a linha em vez de a reler, e é onde o
// primeiro scan gasta o seu tempo.
bool trata_arquivo(const Achado& achado, Biblioteca& antigo, Escriba& escriba,
                   Progresso* progresso) {
  std::error_code erro;
  const std::filesystem::path& caminho = achado.first;
  const std::uintmax_t tamanho = std::filesystem::file_size(caminho, erro);
  if (erro) { ++progresso->desaparecidas; return true; }
  const auto hora = std::filesystem::last_write_time(caminho, erro);
  if (erro) { ++progresso->desaparecidas; return true; }
  ++progresso->vistas;

  Faixa velha;
  const std::int64_t marca =
      static_cast<std::int64_t>(hora.time_since_epoch().count());
  if (antigo.acha_por_caminho(caminho.string(), velha) &&
      velha.modificado == marca &&
      velha.tamanho == static_cast<std::int64_t>(tamanho)) {
    ++progresso->reaproveitadas;
    return escriba.grava(velha);
  }

  Faixa faixa = deriva_do_caminho(caminho, achado.second);
  faixa.modificado = marca;
  faixa.tamanho = static_cast<std::int64_t>(tamanho);
  if (!le_etiqueta(caminho, &faixa)) {
    // Faixa de VIDEO entra ainda que a taglib a recuse, e o `.mkv` recusa-se
    // sempre: a taglib não lê Matroska, e chamar-lhe «não é audio» seria mentir. O
    // que entra é o que o CAMINHO diz, sem etiqueta que se lhe sobreponha, e a
    // duração fica em zero. Fica declarado: a tabella mostra tempo vazio para
    // video, e medi-lo pediria um processo por arquivo em cada varredura.
    if (!extensao_com_video(caminho.extension().string())) {
      ++progresso->recusadas;  // tem extensão de audio, mas não é audio
      return true;
    }
  }
  ++progresso->lidas;
  return escriba.grava(faixa);
}

bool extensao_de_audio(std::string_view extensao) {
  const std::string baixa = minuscula(extensao);
  return std::find(std::begin(kExtensoes), std::end(kExtensoes), baixa) !=
         std::end(kExtensoes);
}

bool extensao_que_interessa(std::string_view extensao) {
  return extensao_de_audio(extensao) || extensao_com_video(extensao);
}

Varredura::Varredura(std::filesystem::path banco,
                     std::vector<std::filesystem::path> raizes,
                     long limite_de_paginas)
    : punho_(std::make_unique<Punho>()) {
  punho_->banco = std::move(banco);
  punho_->raizes = std::move(raizes);

  // O banco antigo abre-se PRIMEIRO, para duas cousas: ver a versão do esquema,
  // e servir de fonte ao reaproveitamento. Ausente não é erro.
  punho_->antigo = std::make_unique<Biblioteca>(punho_->banco);
  if (punho_->antigo->aberta() &&
      punho_->antigo->versao() > kVersaoDoEsquema) {
    // Banco lavrado por um mysong mais novo. NADA se toca: nem se abre escriba,
    // donde temporario algum chega a existir, e o arquivo fica byte a byte.
    punho_->desfecho = Desfecho::EsquemaMaisNovo;
    punho_->fase = Punho::Fase::Fim;
    return;
  }

  punho_->escriba =
      std::make_unique<Escriba>(punho_->banco, limite_de_paginas);
  if (!punho_->escriba->aberto()) {
    punho_->desfecho = Desfecho::ErroDeEscripta;
    punho_->fase = Punho::Fase::Fim;
  }
}

bool Varredura::passo() {
  Punho& punho = *punho_;
  switch (punho.fase) {
    case Punho::Fase::Listar:
      if (punho.raiz_corrente < punho.raizes.size()) {
        lista_raiz(punho.raizes[punho.raiz_corrente++], &punho.progresso,
                   &punho.vistos, &punho.achados);
        return true;
      }
      punho.fase = Punho::Fase::Ler;
      return true;

    case Punho::Fase::Ler:
      if (punho.arquivo_corrente < punho.achados.size()) {
        if (!trata_arquivo(punho.achados[punho.arquivo_corrente++],
                           *punho.antigo, *punho.escriba, &punho.progresso)) {
          // O SQLite recusou: o índice anterior fica, e o temporario some.
          punho.escriba.reset();
          punho.desfecho = Desfecho::ErroDeEscripta;
          punho.fase = Punho::Fase::Fim;
          return false;
        }
        return true;
      }
      punho.fase = Punho::Fase::Concluir;
      return true;

    case Punho::Fase::Concluir:
      punho.desfecho = punho.escriba->conclui() ? Desfecho::Concluido
                                                : Desfecho::ErroDeEscripta;
      punho.escriba.reset();
      punho.fase = Punho::Fase::Fim;
      return false;

    case Punho::Fase::Fim:
      return false;
  }
  return false;
}

Varredura::~Varredura() = default;

Desfecho Varredura::desfecho() const noexcept { return punho_->desfecho; }

const Progresso& Varredura::progresso() const noexcept {
  return punho_->progresso;
}

void Varredura::abandona() {
  punho_->escriba.reset();  // o destructor do Escriba desfaz o temporario
  punho_->fase = Punho::Fase::Fim;
  if (punho_->desfecho == Desfecho::NaoComecou)
    punho_->desfecho = Desfecho::Abandonado;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
