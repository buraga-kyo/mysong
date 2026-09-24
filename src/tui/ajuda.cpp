// ══════════════════════════════════════════════════════════════════════════
//   O CORPO DA AJUDA, src/tui/ajuda.cpp
// ══════════════════════════════════════════════════════════════════════════
// A lavra do que ajuda.hpp promette: a taboada, a legenda, a conta da janella
// e a pintura. Nada aqui lê o mundo: nem tocador, nem banco, nem relogio.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/ajuda.hpp"

#include <algorithm>
#include <utility>

#include <ftxui/screen/string.hpp>

#include "nucleo/espectro.hpp"
#include "tui/espectro.hpp"

namespace mysong::tui {

std::vector<GrupoDaAjuda> taboada_da_ajuda() {
  return {
      {"TOCADOR",
       {{"espaço  F7", "pausa tocando, retoma pausado"},
        {"n  F8", "faixa seguinte"},
        {"p  F6", "faixa anterior"},
        {".  ,", "busca 5 s para a frente, para trás"},
        {"+  -", "volume, por degrau de cinco"},
        {"F11  F10", "volume, por degrau de cinco"},
        {"F9", "cala o som e devolve o que havia"},
        {"z", "liga e desliga o embaralhar"},
        {"x", "repetir: nenhuma, uma, todas"},
        {"l", "esconde e mostra a letra"},
        {"v", "abre a faixa em janela de vídeo"}}},
      {"NAVEGAÇÃO",
       {{"↑  ↓  ←  →", "anda pelo layout, de peça em peça"},
        {"Enter  espaço", "aperta a peça com foco"},
        {"Esc  Backspace", "volta um degrau"},
        {"1  2  3", "vai a MY SONG, PLAYLISTS, DOWNLOAD"},
        {"Tab  Shift+Tab", "cicla as três abas"},
        {"o", "vista: faixas, artistas, álbuns"},
        {"j  k", "anda na lista"},
        {"g  G  Home  End", "ao princípio, ao fim da lista"},
        {"/", "filtra a lista à vista"},
        {"r", "varre o acervo outra vez"},
        {"?  F1", "abre e fecha este HELP"},
        {"q", "sai"}}},
      {"FAIXA",
       {{"F2", "renomeia a faixa eleita"},
        {"Delete", "manda a faixa à lixeira (pergunta)"},
        {"m", "abre o menu de contexto da faixa"},
        {"a", "junta a faixa eleita à lista alvo"},
        {"s  S", "responde sim à pergunta do apagar"}}},
      {"PLAYLISTS",
       {{"P", "as listas"},
        {"c", "cria lista (pede o nome)"},
        {"R", "renomeia a lista"},
        {"D", "apaga a lista (pergunta)"},
        {"t", "retira o item eleito da lista"},
        {"K  J", "move o item para cima, para baixo"}}},
      {"DOWNLOAD",
       {{"s", "busca na rede; Enter baixa o achado"},
        {"f", "fonte: YouTube, YT Music, Spotify"},
        {"A", "trava ou destrava as animações"},
        {"b", "baixa faixa ou playlist por URL"},
        {"I", "lê uma playlist pública do Spotify"},
        {"T", "baixa todas as faixas da lista lida"},
        {"C", "limpa downloads concluídos e falhos"}}},
      {"RATO",
       {{"clique", "aba, faixa, botões: o que dizem"},
        {"clique", "na onda busca; na capa pausa"},
        {"direito", "menu de contexto da faixa"},
        {"roda", "anda três linhas na pauta"},
        {"arrastar", "pega a faixa e larga onde ela vai"}}},
  };
}

std::string rotulo_da_tecla(const ftxui::Event& tecla) {
  namespace f = ftxui;
  if (tecla == f::Event::Return) return "Enter";
  if (tecla == f::Event::Escape) return "Esc";
  if (tecla == f::Event::Backspace) return "Backspace";
  if (tecla == f::Event::Tab) return "Tab";
  if (tecla == f::Event::TabReverse) return "Shift+Tab";
  if (tecla == f::Event::Delete) return "Delete";
  if (tecla == f::Event::ArrowUp) return "↑";
  if (tecla == f::Event::ArrowDown) return "↓";
  if (tecla == f::Event::ArrowLeft) return "←";
  if (tecla == f::Event::ArrowRight) return "→";
  if (tecla == f::Event::Home) return "Home";
  if (tecla == f::Event::End) return "End";
  if (tecla == f::Event::PageUp) return "PgUp";
  if (tecla == f::Event::PageDown) return "PgDn";
  const f::Event funccao[12] = {f::Event::F1, f::Event::F2,  f::Event::F3,
                                f::Event::F4, f::Event::F5,  f::Event::F6,
                                f::Event::F7, f::Event::F8,  f::Event::F9,
                                f::Event::F10, f::Event::F11, f::Event::F12};
  for (int i = 0; i < 12; ++i)
    if (tecla == funccao[i]) return "F" + std::to_string(i + 1);
  if (tecla.is_character())
    return tecla.character() == " " ? "espaço" : tecla.character();
  return {};
}

namespace {

// hertz, o numero em palavra de gente: «250 Hz», «1 kHz», «16 kHz».
std::string hertz(float valor) {
  const int inteiro = static_cast<int>(valor + 0.5f);
  if (inteiro < 1000) return std::to_string(inteiro) + " Hz";
  return std::to_string(inteiro / 1000) + " kHz";
}

}  // namespace

std::vector<AmostraDaAjuda> legenda_do_espectro() {
  std::vector<AmostraDaAjuda> legenda;
  legenda.push_back({tokens::rgb(tokens::v500), "A BARRA", "",
                     "violeta, escura no pé e viva no topo"});
  legenda.push_back({tokens::rgb(tokens::glow_hot), "A BATIDA", "",
                     "o topo da barra acende em rosa"});
  const struct {
    Registro registro;
    float desde, ate;
    const char* nota;
  } quatro[4] = {
      {Registro::Graves, nucleo::HERTZ_MINIMO, FRONTEIRA_DOS_GRAVES,
       "bumbo, baixo"},
      {Registro::MediosGraves, FRONTEIRA_DOS_GRAVES,
       FRONTEIRA_DOS_MEDIOS_GRAVES, "caixa, guitarra, o corpo da voz"},
      {Registro::MediosAgudos, FRONTEIRA_DOS_MEDIOS_GRAVES,
       FRONTEIRA_DOS_MEDIOS_AGUDOS, "voz, presença, teclados"},
      {Registro::Agudos, FRONTEIRA_DOS_MEDIOS_AGUDOS, nucleo::HERTZ_MAXIMO,
       "pratos, chimbal, o ar"}};
  // Os quatro registros vão SEM côr propria (issue #167): o que elles dizem é
  // em que faixa de hertz cada columna sôa, e a côr da batida é uma só.
  for (const auto& q : quatro)
    legenda.push_back({tokens::rgb(tokens::text_body),
                       std::string(nome_do_registro(q.registro)),
                       hertz(q.desde) + " a " + hertz(q.ate), q.nota});
  legenda.push_back(
      {tokens::rgb(tokens::text_faint), "MUDO", "", "e o silêncio: apagado"});
  return legenda;
}

namespace {

constexpr std::size_t kTecla = 16;
constexpr std::size_t kFaz = LARGURA_DA_COLLUNHA - kTecla - 1;
constexpr std::string_view kAmostra = "▂▄▆█ ";
constexpr std::size_t kLarguraDaAmostra = 5;

ftxui::Color cor(tokens::Triade t) { return ftxui::Color::RGB(t.r, t.g, t.b); }
ftxui::Color cor(std::string_view hex) { return cor(tokens::rgb(hex)); }

// aparar, o texto á ESQUERDA da caixa de `largura` collunhas: enche de espaço
// o que sobra, e corta com «…» o que não cabe. Mede-se em collunhas do
// terminal, e não em pontos de codigo, que a seta e o acento não são um byte.
std::string aparar(const std::string& texto, std::size_t largura) {
  if (largura == 0) return {};
  const std::size_t inteiro =
      static_cast<std::size_t>(ftxui::string_width(texto));
  if (inteiro <= largura)
    return texto + std::string(largura - inteiro, ' ');
  std::string feito;
  std::size_t gastas = 0;
  for (std::size_t i = 0; i < texto.size();) {
    std::size_t fim = i + 1;
    while (fim < texto.size() &&
           (static_cast<unsigned char>(texto[fim]) & 0xC0) == 0x80)
      ++fim;
    const std::string letra = texto.substr(i, fim - i);
    const std::size_t vale =
        static_cast<std::size_t>(ftxui::string_width(letra));
    if (gastas + vale > largura - 1) break;
    feito += letra;
    gastas += vale;
    i = fim;
  }
  return feito + "…" + std::string(largura - gastas - 1, ' ');
}

using Fileiras = std::vector<ftxui::Element>;

ftxui::Element fileira_em_branco() {
  return ftxui::text(std::string(LARGURA_DA_COLLUNHA, ' '));
}

ftxui::Element cabeca_do_grupo(const std::string& nome) {
  return ftxui::text(aparar(nome, LARGURA_DA_COLLUNHA)) | ftxui::bold |
         ftxui::color(cor(tokens::text_heading));
}

Fileiras fileiras_do_grupo(const GrupoDaAjuda& grupo) {
  Fileiras fileiras;
  fileiras.push_back(cabeca_do_grupo(grupo.nome));
  for (const LinhaDaAjuda& linha : grupo.linhas)
    fileiras.push_back(ftxui::hbox(
        {ftxui::text(aparar(linha.tecla, kTecla)) | ftxui::bold |
             ftxui::color(cor(tokens::glow_soft)),
         ftxui::text(" "),
         ftxui::text(aparar(linha.faz, kFaz)) |
             ftxui::color(cor(tokens::text_body))}));
  return fileiras;
}

// As tres linhas que dizem a regra, antes das amostras. Dizem o que a issue
// #132 faz: a barra é uma só, e a côr é da batida forte e do registro d'ella.
constexpr const char* kRegra[3] = {
    "a barra é violeta, escura no pé e viva no topo;",
    "na batida mais forte de uma banda, a coluna inteira",
    "acende na cor do registro dela:"};

Fileiras fileiras_da_legenda() {
  Fileiras fileiras;
  fileiras.push_back(cabeca_do_grupo("ESPECTRO"));
  for (const char* linha : kRegra)
    fileiras.push_back(ftxui::text(aparar(linha, LARGURA_DA_COLLUNHA)) |
                       ftxui::color(cor(tokens::text_body)));
  const std::size_t resto = LARGURA_DA_COLLUNHA - kLarguraDaAmostra;
  for (const AmostraDaAjuda& amostra : legenda_do_espectro()) {
    const std::string cabeca =
        amostra.faixa.empty() ? amostra.rotulo
                              : amostra.rotulo + "  " + amostra.faixa;
    fileiras.push_back(ftxui::hbox(
        {ftxui::text(std::string(kAmostra)) | ftxui::color(cor(amostra.tinta)),
         ftxui::text(aparar(cabeca, resto)) | ftxui::bold |
             ftxui::color(cor(tokens::text_primary))}));
    fileiras.push_back(ftxui::text(std::string(kLarguraDaAmostra, ' ') +
                                   aparar(amostra.nota, resto)) |
                       ftxui::color(cor(tokens::text_muted)));
  }
  return fileiras;
}

// A PAGINA: os blocos repartidos em collunhas CONTIGUAS, na ordem de leitura,
// pela repartição que dá a collunha mais alta MAIS BAIXA. Sete blocos e tres
// collunhas dão quinze cortes possiveis: experimentam-se todos.
struct Pagina {
  std::vector<Fileiras> collunhas;
  std::size_t conteudo = 0;  // a altura da collunha mais alta
};

void procura_cortes(const std::vector<std::size_t>& alturas,
                    std::size_t desde, std::size_t collunhas_que_faltam,
                    std::vector<std::size_t>& cortes, std::size_t pior_ate_aqui,
                    std::vector<std::size_t>& melhores, std::size_t& melhor) {
  const std::size_t quantos = alturas.size();
  if (collunhas_que_faltam == 1) {
    std::size_t somma = 0;
    for (std::size_t b = desde; b < quantos; ++b) somma += alturas[b];
    const std::size_t pior = std::max(pior_ate_aqui, somma);
    if (pior < melhor) {
      melhor = pior;
      melhores = cortes;
    }
    return;
  }
  std::size_t somma = 0;
  for (std::size_t corte = desde + 1; corte < quantos; ++corte) {
    somma += alturas[corte - 1];
    cortes.push_back(corte);
    procura_cortes(alturas, corte, collunhas_que_faltam - 1, cortes,
                   std::max(pior_ate_aqui, somma), melhores, melhor);
    cortes.pop_back();
  }
}

Pagina pagina_da_ajuda(std::size_t collunhas) {
  std::vector<Fileiras> blocos;
  for (const GrupoDaAjuda& grupo : taboada_da_ajuda())
    blocos.push_back(fileiras_do_grupo(grupo));
  blocos.push_back(fileiras_da_legenda());
  // Um bloco por collunha no maximo: mais collunhas que blocos ficariam vazias.
  collunhas = std::max<std::size_t>(1, std::min(collunhas, blocos.size()));
  std::vector<std::size_t> alturas;
  for (const Fileiras& bloco : blocos) alturas.push_back(bloco.size() + 1);
  std::vector<std::size_t> cortes, melhores;
  std::size_t melhor = static_cast<std::size_t>(-1);
  procura_cortes(alturas, 0, collunhas, cortes, 0, melhores, melhor);
  melhores.push_back(blocos.size());
  Pagina pagina;
  std::size_t desde = 0;
  for (const std::size_t corte : melhores) {
    Fileiras collunha;
    for (std::size_t b = desde; b < corte; ++b) {
      // A fileira em branco aparta os blocos, e não sobra no fim da collunha.
      if (!collunha.empty()) collunha.push_back(fileira_em_branco());
      collunha.insert(collunha.end(), blocos[b].begin(), blocos[b].end());
    }
    pagina.conteudo = std::max(pagina.conteudo, collunha.size());
    pagina.collunhas.push_back(std::move(collunha));
    desde = corte;
  }
  return pagina;
}

// As tres fileiras que a janella gasta além do conteudo: a orla de cima, o
// rodapé e a orla de baixo. E as quatro collunhas: a orla e o vão dos lados.
constexpr std::size_t kFileirasDaMoldura = 3;
constexpr std::size_t kCollunhasDaMoldura = 4;

ftxui::Element vao(ftxui::WidthOrHeight qual, std::size_t quanto) {
  return ftxui::emptyElement() |
         ftxui::size(qual, ftxui::EQUAL, static_cast<int>(quanto));
}

}  // namespace

MedidaDaAjuda medida_da_ajuda(std::size_t largura_da_tela,
                              std::size_t altura_da_tela) {
  MedidaDaAjuda medida;
  if (largura_da_tela < kCollunhasDaMoldura + 1 ||
      altura_da_tela < kFileirasDaMoldura + 2)
    return medida;
  const std::size_t util = largura_da_tela - kCollunhasDaMoldura;
  medida.collunhas = std::clamp<std::size_t>(
      (util + VAO_ENTRE_COLLUNHAS) / (LARGURA_DA_COLLUNHA + VAO_ENTRE_COLLUNHAS),
      1, MAXIMO_DE_COLLUNHAS);
  const Pagina pagina = pagina_da_ajuda(medida.collunhas);
  medida.collunhas = pagina.collunhas.size();
  medida.conteudo = pagina.conteudo;
  medida.largura = std::min(
      largura_da_tela, medida.collunhas * LARGURA_DA_COLLUNHA +
                           (medida.collunhas - 1) * VAO_ENTRE_COLLUNHAS +
                           kCollunhasDaMoldura);
  medida.altura = std::min(altura_da_tela, medida.conteudo + kFileirasDaMoldura);
  medida.uteis = medida.altura - kFileirasDaMoldura;
  medida.x = static_cast<int>((largura_da_tela - medida.largura) / 2);
  medida.y = static_cast<int>((altura_da_tela - medida.altura) / 2);
  return medida;
}

std::size_t rolagem_maxima(const MedidaDaAjuda& medida) noexcept {
  return medida.conteudo > medida.uteis ? medida.conteudo - medida.uteis : 0;
}

void alterna_a_ajuda(Ajuda& ajuda) noexcept {
  ajuda.aberta = !ajuda.aberta;
  ajuda.rolagem = 0;
}

namespace {

void rola(Ajuda& ajuda, long passo, std::size_t maxima) noexcept {
  const long alvo = static_cast<long>(std::min(ajuda.rolagem, maxima)) + passo;
  ajuda.rolagem = static_cast<std::size_t>(
      std::clamp<long>(alvo, 0, static_cast<long>(maxima)));
}

}  // namespace

bool tecla_na_ajuda(Ajuda& ajuda, const ftxui::Event& tecla,
                    std::size_t maxima) noexcept {
  namespace f = ftxui;
  if (!ajuda.aberta) return false;
  if (tecla == f::Event::Escape || tecla == f::Event::Character('?') ||
      tecla == f::Event::F1 || tecla == f::Event::Return ||
      tecla == f::Event::Backspace || tecla == f::Event::Character('q')) {
    ajuda.aberta = false;
    ajuda.rolagem = 0;
    return true;
  }
  if (tecla == f::Event::ArrowDown || tecla == f::Event::Character('j'))
    rola(ajuda, 1, maxima);
  else if (tecla == f::Event::ArrowUp || tecla == f::Event::Character('k'))
    rola(ajuda, -1, maxima);
  else if (tecla == f::Event::PageDown)
    rola(ajuda, static_cast<long>(PASSO_DA_PAGINA), maxima);
  else if (tecla == f::Event::PageUp)
    rola(ajuda, -static_cast<long>(PASSO_DA_PAGINA), maxima);
  else if (tecla == f::Event::Home)
    ajuda.rolagem = 0;
  else if (tecla == f::Event::End)
    ajuda.rolagem = maxima;
  return true;  // aberta, toda tecla morre aqui
}

bool rato_na_ajuda(Ajuda& ajuda, const ftxui::Box& caixa,
                   const ftxui::Mouse& rato, std::size_t maxima) noexcept {
  if (!ajuda.aberta) return false;
  if (rato.motion != ftxui::Mouse::Pressed) return true;
  if (rato.button == ftxui::Mouse::WheelUp) {
    rola(ajuda, -3, maxima);
  } else if (rato.button == ftxui::Mouse::WheelDown) {
    rola(ajuda, 3, maxima);
  } else if (!caixa.Contain(rato.x, rato.y)) {
    ajuda.aberta = false;
    ajuda.rolagem = 0;
  }
  return true;
}

ftxui::Element flutuante_da_ajuda(const Ajuda& ajuda,
                                  std::size_t largura_da_tela,
                                  std::size_t altura_da_tela,
                                  ftxui::Box* caixa) {
  if (caixa != nullptr) *caixa = ftxui::Box{0, -1, 0, -1};
  if (!ajuda.aberta) return ftxui::emptyElement();
  const MedidaDaAjuda medida = medida_da_ajuda(largura_da_tela, altura_da_tela);
  if (medida.largura == 0) return ftxui::emptyElement();
  const Pagina pagina = pagina_da_ajuda(medida.collunhas);
  const std::size_t maxima = rolagem_maxima(medida);
  const std::size_t desde = std::min(ajuda.rolagem, maxima);
  std::vector<ftxui::Element> collunhas;
  collunhas.push_back(vao(ftxui::WIDTH, 1));
  for (std::size_t c = 0; c < pagina.collunhas.size(); ++c) {
    const Fileiras& toda = pagina.collunhas[c];
    Fileiras visiveis;
    for (std::size_t i = desde; i < desde + medida.uteis; ++i)
      visiveis.push_back(i < toda.size() ? toda[i] : fileira_em_branco());
    if (c > 0) collunhas.push_back(vao(ftxui::WIDTH, VAO_ENTRE_COLLUNHAS));
    collunhas.push_back(ftxui::vbox(std::move(visiveis)) |
                        ftxui::size(ftxui::WIDTH, ftxui::EQUAL,
                                    static_cast<int>(LARGURA_DA_COLLUNHA)));
  }
  collunhas.push_back(vao(ftxui::WIDTH, 1));
  const std::string rodape =
      maxima > 0 ? "Esc ou ? fecha  ·  ↑ ↓ rola" : "Esc ou ? fecha";
  ftxui::Element janella =
      ftxui::window(ftxui::text(" HELP ") | ftxui::bold |
                        ftxui::color(cor(tokens::text_heading)),
                    ftxui::vbox({ftxui::hbox(std::move(collunhas)),
                                 ftxui::text(rodape) |
                                     ftxui::color(cor(tokens::text_muted)) |
                                     ftxui::center})) |
      ftxui::color(cor(tokens::line_base)) |
      ftxui::bgcolor(cor(tokens::panel)) |
      ftxui::size(ftxui::WIDTH, ftxui::EQUAL, static_cast<int>(medida.largura)) |
      ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, static_cast<int>(medida.altura));
  if (caixa != nullptr) janella = janella | ftxui::reflect(*caixa);
  return ftxui::vbox({vao(ftxui::HEIGHT, static_cast<std::size_t>(medida.y)),
                      ftxui::hbox({vao(ftxui::WIDTH,
                                       static_cast<std::size_t>(medida.x)),
                                   std::move(janella)}),
                      ftxui::filler()});
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO., buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
