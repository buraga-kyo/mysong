// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA TABELLA — src/tui/tabella.cpp
// ══════════════════════════════════════════════════════════════════════════
// A pintura. Vale a regra do cabeçalho: pinta e sahe.
//
// DOMÍNIO ......... o Navegador por leitura, e a geometria.
// CONTRA-DOMÍNIO .. elementos do FTXUI.
// INVARIANTE ...... funcção alguma d'aqui muta o navegador. O parametro é
//                   `const&`, e o compilador guarda a regra.
// Q.E.D. .......... a decisão toda vive no navegador, e é lá que se prova; aqui
//                   sómente se traduz estado em tinta.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/tabella.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/screen/string.hpp>

#include "tui/rato.hpp"
#include "tui/tokens.hpp"
#include "tui/transporte.hpp"

namespace mysong::tui {

namespace {

ftxui::Element pinta(const std::string& texto, std::string_view token) {
  const tokens::Triade c = tokens::rgb(token);
  return ftxui::text(texto) |
         ftxui::color(ftxui::Color::RGB(c.r, c.g, c.b));
}

// cortar — a cadeia nos primeiros `largura` CODEPOINTS, e não bytes nem
// collunhas: o glypho largo (CJK, emoji) conta por um valendo duas. Sem a
// conta por codepoint, um titulo com acentos sahiria mais curto do que a conta
// diz e a tabella perderia o alinhamento das columnas.
//
// NÃO enche o que sobra, e o enchimento é que ficou no `apara`: quem põe caret
// no fim do texto quer o corte nú, que espaço á direita empurraria o caret uma
// collunha para lá do que se escreveu.
//
// Devolve tambem, por `deixadas`, quantas contou: quem enche o que sobra já não
// tem de tornar a percorrer a cadeia para o saber, e a volta pelo UTF-8 fica
// n'este logar só, que foi a razão de se partir o `apara` em dous.
std::string cortar(const std::string& crua, std::size_t largura,
                   std::size_t* deixadas = nullptr) {
  std::string feita;
  std::size_t contadas = 0;
  for (std::size_t i = 0; i < crua.size(); ++i) {
    if ((static_cast<unsigned char>(crua[i]) & 0xC0) != 0x80) {
      if (contadas == largura) break;
      ++contadas;
    }
    feita += crua[i];
  }
  if (deixadas != nullptr) *deixadas = contadas;
  return feita;
}

// apara — o corte, enchido de espaços até `largura`. É o enchimento que alinha
// as columnas da tabella, e é por isso que elle existe.
std::string apara(const std::string& crua, std::size_t largura) {
  std::size_t contadas = 0;
  std::string feita = cortar(crua, largura, &contadas);
  while (contadas++ < largura) feita += ' ';
  return feita;
}

// As cellas das columnas de largura fixa. A margem de UMA cella de cada lado é
// o que aparta a pauta da orla e do divisor sem gastar collunha de traço.
constexpr std::size_t kMargem = 1, kMarcador = 1, kNumero = 3, kVao = 2;
constexpr std::size_t kRegua = 6, kTempo = 5, kConta = 4;
// kTituloMinimo — abaixo d'isto o titulo não diz nada, e columna nova que o
// levasse a menos seria columna que cega a linha para enfeitar a folha.
constexpr std::size_t kTituloMinimo = 8;

// repete — o glypho tantas vezes. Não vale `std::string(n, c)`: o glypho da
// régua tem tres octetos, e aquelle constructor repete OCTETO, d'onde sahiria
// lixo em vez de barra.
std::string repete(std::string_view glypho, std::size_t quantas) {
  std::string feita;
  for (std::size_t i = 0; i < quantas; ++i) feita += glypho;
  return feita;
}

// a_direita — o texto encostado á DIREITA da columna. O № e o tempo lêem-se
// pela ultima cella, e alinhal-os á esquerda faria a vista saltar de linha
// para linha conforme o numero tivesse um algarismo ou tres.
std::string a_direita(const std::string& texto, std::size_t collunhas) {
  const std::size_t mede = static_cast<std::size_t>(ftxui::string_width(texto));
  if (mede >= collunhas) return apara_collunhas(texto, collunhas);
  return std::string(collunhas - mede, ' ') + texto;
}

}  // namespace

Medidas medidas_da_pauta(std::size_t largura, bool ha_autor, bool pela_conta) {
  Medidas medidas;
  medidas.pela_conta = pela_conta;
  // A pauta MINIMA: as duas margens e o titulo, e mais nada.
  if (largura < 2 * kMargem + kMarcador + kTituloMinimo) {
    medidas.titulo = largura > 2 * kMargem ? largura - 2 * kMargem : largura;
    return medidas;
  }
  medidas.marcador = kMarcador;
  std::size_t sobra = largura - 2 * kMargem - kMarcador;
  // O № é numero DE FAIXA: na vista que conta nomes elle não existe, e a conta
  // d'ella vae na columna da direita.
  if (!pela_conta && sobra >= kNumero + kVao + kTituloMinimo) {
    medidas.numero = kNumero;
    sobra -= kNumero + kVao;
  }
  const std::size_t direita = pela_conta ? kConta : kTempo;
  if (sobra >= direita + 1 + kTituloMinimo) {
    medidas.conta = direita;
    sobra -= direita + 1;
  }
  if (sobra >= kRegua + kVao + kTituloMinimo) {
    medidas.regua = kRegua;
    sobra -= kRegua + kVao;
  }
  // DOUS TERÇOS ao titulo e UM ao artista, que é o que a issue #111 pede. O
  // artista é a primeira a ceder por ser a unica columna que o titulo já
  // costuma dizer: «97Kickstvr, without you» traz o nome dentro.
  if (!pela_conta && ha_autor && sobra >= kVao + 3 * kTituloMinimo) {
    medidas.artista = (sobra - kVao) / 3;
    sobra -= kVao + medidas.artista;
  }
  medidas.titulo = sobra;
  return medidas;
}

Medidas medidas_da_fatia(const Navegador& navegador, std::size_t primeira,
                         std::size_t fim, std::size_t largura, int* maior) {
  // A vista que CONTA nomes: alli o numero da linha é conta de faixas, e não
  // numero de faixa, e duração não ha nenhuma.
  const Secao secao = navegador.secao();
  const bool pela_conta = secao == Secao::Artistas || secao == Secao::Albuns ||
                          secao == Secao::Rois;
  const std::vector<Linha>& vista = navegador.vista();
  bool ha_autor = false;
  int maior_da_fatia = 0;
  for (std::size_t i = primeira; i < fim && i < vista.size(); ++i) {
    if (!vista[i].autor.empty()) ha_autor = true;
    maior_da_fatia = std::max(
        maior_da_fatia, pela_conta ? vista[i].numero : vista[i].duracao);
  }
  if (maior != nullptr) *maior = maior_da_fatia;
  return medidas_da_pauta(largura, ha_autor, pela_conta);
}

std::size_t cheias_da_regua(int quanto, int maior, std::size_t cellas) {
  if (quanto <= 0 || maior <= 0 || cellas == 0) return 0;
  const std::size_t medida = static_cast<std::size_t>(quanto);
  const std::size_t tecto = static_cast<std::size_t>(maior);
  if (medida >= tecto) return cellas;
  // O dobro no numerador e no denominador é o arredondamento ao mais proximo
  // feito em inteiros: sommar meia cella antes de dividir.
  const std::size_t cheias = (medida * cellas * 2 + tecto) / (tecto * 2);
  return cheias == 0 ? 1 : cheias;
}

ftxui::Element elemento_da_linha(const std::vector<Pedaco>& pedacos,
                                 bool eleita, bool soa, std::size_t largura) {
  const std::string_view sobre = soa ? tokens::panel : tokens::v50;
  std::vector<ftxui::Element> partes;
  partes.reserve(pedacos.size());
  for (const Pedaco& pedaco : pedacos) {
    ftxui::Element parte = pinta(pedaco.texto, eleita ? sobre : pedaco.tinta);
    if (pedaco.negrito) parte = parte | ftxui::bold;
    partes.push_back(std::move(parte));
  }
  ftxui::Element linha = ftxui::hbox(std::move(partes));
  if (eleita) {
    const tokens::Triade fundo =
        tokens::rgb(soa ? tokens::glow_core : tokens::v600);
    linha = linha |
            ftxui::bgcolor(ftxui::Color::RGB(fundo.r, fundo.g, fundo.b));
  }
  // O cinge da largura é o que faz o bloco chegar á orla mesmo onde a somma
  // dos pedaços desse menos: fundo que parasse a meio lê-se como defeito.
  return linha |
         ftxui::size(ftxui::WIDTH, ftxui::EQUAL, static_cast<int>(largura));
}

std::vector<Pedaco> pedacos_da_linha(const Linha& linha,
                                     const Medidas& medidas, int maior,
                                     bool soa) {
  std::vector<Pedaco> feitos;
  const auto vao = [&feitos](std::size_t quantas) {
    if (quantas > 0)
      feitos.push_back({std::string(quantas, ' '), tokens::text_faint, false});
  };
  vao(kMargem);
  // O «▶» tem cella PROPRIA, e não toma o logar do №: tomando-o, a linha que
  // sôa perdia o numero d'ella, e o operador que conta pela pauta perdia a
  // conta justamente na linha que está a ouvir.
  if (medidas.marcador > 0)
    feitos.push_back({soa ? "\u25b6" : " ", tokens::glow_core, false});
  if (medidas.numero > 0) {
    const std::string numero =
        linha.numero > 0 ? std::to_string(linha.numero) : std::string();
    feitos.push_back(
        {a_direita(numero, medidas.numero), tokens::text_faint, false});
    vao(kVao);
  }
  // O titulo da que SÔA accende sem que o resto da linha accenda: são dous
  // signaes apartados, e o outro, o da eleita, é o bloco inteiro.
  feitos.push_back({apara_collunhas(linha.texto, medidas.titulo),
                    soa ? tokens::glow_soft : tokens::text_primary, true});
  if (medidas.artista > 0) {
    vao(kVao);
    feitos.push_back({apara_collunhas(linha.autor, medidas.artista),
                      tokens::text_body, false});
  }
  // O que a columna da direita diz, e o que a régua mede: na faixa é a duração,
  // e na vista que conta nomes é a conta de faixas.
  const int quanto = medidas.pela_conta ? linha.numero : linha.duracao;
  const std::string direita =
      quanto <= 0                ? std::string()
      : medidas.pela_conta       ? std::to_string(quanto)
                                 : mm_ss(linha.duracao);
  const auto a_conta = [&](std::size_t vao_antes) {
    if (medidas.conta == 0) return;
    vao(vao_antes);
    feitos.push_back(
        {a_direita(direita, medidas.conta), tokens::text_muted, false});
  };
  // Na vista que conta nomes a conta vem ANTES da régua: é ella que a régua
  // mede, e ler o desenho antes do numero seria ler a legenda depois do mappa.
  if (medidas.pela_conta) a_conta(1);
  if (medidas.regua > 0) {
    vao(kVao);
    const std::size_t cheias = cheias_da_regua(quanto, maior, medidas.regua);
    feitos.push_back({repete("\u25b0", cheias), tokens::v700, false});
    feitos.push_back(
        {repete("\u25b1", medidas.regua - cheias), tokens::line_faint, false});
  }
  if (!medidas.pela_conta) a_conta(1);
  vao(kMargem);
  return feitos;
}

std::string apara_collunhas(const std::string& crua, std::size_t collunhas) {
  if (collunhas == 0) return {};
  if (static_cast<std::size_t>(ftxui::string_width(crua)) <= collunhas) {
    std::string feita = crua;
    while (static_cast<std::size_t>(ftxui::string_width(feita)) < collunhas)
      feita += ' ';
    return feita;
  }
  // O `Utf8ToGlyphs` do FTXUI devolve UM item por CELLA: depois do glypho largo
  // vem um item VAZIO, que é a segunda cella d'elle. É por elle que se conta, e
  // não pelos octetos, que é o que faz o corte casar com o que a tela mostra.
  const std::vector<std::string> glyphos = ftxui::Utf8ToGlyphs(crua);
  const std::size_t cabem = collunhas - 1;  // uma cella fica para o «…»
  std::string feita;
  std::size_t gastas = 0;
  for (std::size_t i = 0; i < glyphos.size(); ++i) {
    if (glyphos[i].empty()) continue;  // a segunda cella do glypho largo
    const std::size_t mede =
        i + 1 < glyphos.size() && glyphos[i + 1].empty() ? 2 : 1;
    if (gastas + mede > cabem) break;
    feita += glyphos[i];
    gastas += mede;
  }
  feita += "\u2026";
  // O glypho largo que não coube deixa UMA cella orphã antes da reticencia: ella
  // enche-se de espaço, que a columna promette largura fixa.
  while (gastas++ + 1 < collunhas) feita += ' ';
  return feita;
}

ftxui::Element caret_do_campo() {
  // Espaço, e não cadeia vazia: o cursor pousa no `x_min` da caixa d'este nó, e
  // nó de largura zero não tem caixa que sirva de endereço.
  return ftxui::text(" ") | ftxui::focusCursorBar;
}

ftxui::Element elemento_da_tabella(const Navegador& navegador,
                                   std::size_t primeira, std::size_t altura,
                                   std::size_t largura,
                                   const std::string& tocando,
                                   std::vector<ftxui::Box>* caixas) {
  // Limpa-se á entrada, e não sómente nos ramos que pintam linhas: sahida
  // antecipada que deixasse as caixas do quadro anterior faria o clique
  // acertar linhas que já não estão na tela.
  if (caixas != nullptr) caixas->clear();
  if (altura == 0 || largura == 0) return ftxui::text("");
  const std::vector<Linha>& vista = navegador.vista();
  if (vista.empty()) {
    // O recado do vazio é POR SECÇÃO. Um recado só dizia «varra o acervo» dentro de
    // uma lista de faixas escolhidas á mão, que é conselho que não serve para nada
    // e manda o operador ao logar errado.
    const char* recado = "  (nada aqui: varra o acervo, ou baixe uma faixa)";
    switch (navegador.secao()) {
      case Secao::Rois:
        recado = "  (lista alguma ainda: `c` cria uma)";
        break;
      case Secao::NoRol:
        recado = "  (lista vazia: elege uma faixa no acervo e tecla `a`)";
        break;
      case Secao::Rede:
        recado = "  (nada achado: `s` pergunta outra vez)";
        break;
      case Secao::Lista:
        recado = "  (lista alguma lida: `I` cola a URL de uma do Spotify)";
        break;
      case Secao::Busca:
        // As MINHAS MÚSICAS abrem com termo VAZIO (issue #93): não havendo
        // termo, quem está vazio é o acervo, e culpar o termo mandaria o
        // operador procurar erro de escripta que elle não commetteu.
        if (!navegador.termo().empty()) recado = "  (nada casa com esse termo)";
        break;
      case Secao::Artistas:
      case Secao::Albuns:
      case Secao::Faixas:
        break;
    }
    return pinta(recado, tokens::text_faint);
  }

  // As columnas e a maior linha da fatia sahem d'uma conta só, que a bateria
  // interroga sem écran. A pintura d'aqui em diante é traducção, e não decisão.
  const std::size_t fim_da_fatia = std::min(primeira + altura, vista.size());
  int maior = 0;
  const Medidas medidas =
      medidas_da_fatia(navegador, primeira, fim_da_fatia, largura, &maior);

  std::vector<ftxui::Element> linhas;
  // Dimensiona-se ANTES do laço, pela razão da barra: o `reflect` guarda
  // referencia, e realloque no meio do quadro deixá-la-hia pendurada.
  // A subtracção vae GUARDADA, como a do transporte: em std::size_t tirar mais
  // do que ha dá numero enorme, e o `assign` tentaria armar bilhões de caixas.
  // Hoje o caso não chega aqui, que a `primeira_a_mostrar` o impede; mas esta
  // funcção é publica, e o laço de baixo já era tolerante ao mesmo engano.
  if (caixas != nullptr)
    caixas->assign(fim_da_fatia > primeira ? fim_da_fatia - primeira : 0,
                   caixa_por_pintar());
  for (std::size_t i = primeira; i < fim_da_fatia; ++i) {
    const Linha& linha = vista[i];
    const bool eleita = i == navegador.eleito();
    // O que SÔA casa-se pela CHAVE, que nas secções de faixa é o caminho do
    // arquivo. Nas outras a chave é nome ou id, e ahi nada casa, que é o que se
    // quer: album algum «toca».
    const bool soa = !tocando.empty() && linha.chave == tocando;
    ftxui::Element pintada = elemento_da_linha(
        pedacos_da_linha(linha, medidas, maior, soa), eleita, soa, largura);
    if (caixas != nullptr)
      pintada = pintada | ftxui::reflect((*caixas)[i - primeira]);
    linhas.push_back(std::move(pintada));
  }
  return ftxui::vbox(std::move(linhas));
}

ftxui::Element elemento_da_letra(const std::vector<nucleo::LinhaDaLetra>& linhas,
                                 int corrente, std::size_t altura,
                                 std::size_t largura) {
  if (altura == 0 || largura == 0) return ftxui::text("");
  if (linhas.empty())
    return pinta(apara("  (sem letra para esta faixa)", largura),
                 tokens::text_faint);

  // A corrente vae no MEIO da janella, e não no alto: o operador lê o que vem, e
  // não sómente o que passou. Fica no alto sómente no principio da musica, e no
  // fim quando já não ha o que vir.
  const std::size_t meio = altura / 2;
  const int alvo = corrente < 0 ? 0 : corrente;
  std::size_t primeira = 0;
  if (static_cast<std::size_t>(alvo) > meio) primeira = alvo - meio;
  if (primeira + altura > linhas.size())
    primeira = linhas.size() > altura ? linhas.size() - altura : 0;

  std::vector<ftxui::Element> pintadas;
  const std::size_t fim = std::min(primeira + altura, linhas.size());
  for (std::size_t i = primeira; i < fim; ++i) {
    const bool esta = static_cast<int>(i) == corrente;
    ftxui::Element linha =
        pinta(apara("  " + linhas[i].texto, largura),
              esta ? tokens::text_bright : tokens::text_faint);
    if (esta) linha = linha | ftxui::bold;
    pintadas.push_back(std::move(linha));
  }
  return ftxui::vbox(std::move(pintadas));
}

ftxui::Element elemento_da_capa(const nucleo::CapaPintada& capa,
                                std::size_t collunas, std::size_t linhas,
                                ftxui::Box* caixa) {
  // Esvazia-se á entrada: terminal apertado não mostra capa alguma, e a caixa
  // do quadro anterior deixaria o clique a pausar sobre a tabella.
  if (caixa != nullptr) *caixa = caixa_por_pintar();
  if (collunas == 0 || linhas == 0) return ftxui::text("");
  const auto lembrar = [caixa](ftxui::Element pintada) {
    return caixa == nullptr ? pintada : pintada | ftxui::reflect(*caixa);
  };
  if (capa.achada) {
    // Cada corrida vira UM elemento com a sua tinta. Não se passa a cadeia crua do
    // chafa: o FTXUI contaria os octetos do escape como LARGURA, e a capa esmagaria a
    // pauta e o painel. Medi-o, e está registrado no tractado da capa.
    std::vector<ftxui::Element> pintadas;
    pintadas.reserve(capa.linhas.size());
    for (const std::vector<nucleo::Corrida>& linha : capa.linhas) {
      std::vector<ftxui::Element> corridas;
      corridas.reserve(linha.size());
      for (const nucleo::Corrida& corrida : linha) {
        ftxui::Element pedaco = ftxui::text(corrida.texto);
        if (corrida.r_frente >= 0)
          pedaco = pedaco | ftxui::color(ftxui::Color::RGB(
                                corrida.r_frente, corrida.g_frente,
                                corrida.b_frente));
        if (corrida.r_fundo >= 0)
          pedaco = pedaco | ftxui::bgcolor(ftxui::Color::RGB(
                                corrida.r_fundo, corrida.g_fundo,
                                corrida.b_fundo));
        corridas.push_back(std::move(pedaco));
      }
      pintadas.push_back(ftxui::hbox(std::move(corridas)));
    }
    return lembrar(ftxui::vbox(std::move(pintadas)));
  }

  // O MARCADOR: uma nota musical no meio de um quadro de orla, com os tokens d'esta
  // Casa. Não é enfeite: album sem capa mostra que NÃO TEM, e não um buraco que o
  // operador tomaria por falha da tela.
  std::vector<ftxui::Element> pintadas;
  const std::size_t meio = linhas / 2;
  for (std::size_t l = 0; l < linhas; ++l) {
    if (l == meio) {
      const std::size_t esquerda = collunas > 1 ? (collunas - 1) / 2 : 0;
      pintadas.push_back(pinta(std::string(esquerda, ' ') + "\u266b",
                               tokens::text_faint));
    } else {
      pintadas.push_back(pinta(std::string(collunas, ' '), tokens::inset));
    }
  }
  return lembrar(ftxui::vbox(std::move(pintadas)) | ftxui::border);
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
