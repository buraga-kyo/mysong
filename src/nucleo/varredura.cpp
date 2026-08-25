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

#include <algorithm>
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

bool extensao_de_audio(std::string_view extensao) {
  const std::string baixa = minuscula(extensao);
  return std::find(std::begin(kExtensoes), std::end(kExtensoes), baixa) !=
         std::end(kExtensoes);
}

Varredura::Varredura(std::filesystem::path banco,
                     std::vector<std::filesystem::path> raizes)
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

  punho_->escriba = std::make_unique<Escriba>(punho_->banco);
  if (!punho_->escriba->aberto()) {
    punho_->desfecho = Desfecho::ErroDeEscripta;
    punho_->fase = Punho::Fase::Fim;
  }
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
