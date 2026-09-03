// ══════════════════════════════════════════════════════════════════════════
//   O CORPO DO DESENHO DO ESPECTRO — src/tui/espectro.cpp
// ══════════════════════════════════════════════════════════════════════════
// A conta que o tractado promette. Nada aqui abre terminal, lê ambiente ou
// consulta relogio: d'onde toda affirmação d'este manuscripto se prova em
// machina surda, e a fita deixa de depender do olho de quem a abriu.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/espectro.hpp"

#include "nucleo/espectro.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace mysong::tui {

namespace {

// cingido — a magnitude reduzida ao intervallo [0,1] que o contracto promette,
// e que esta Casa não confia. A ORDEM das duas guardas é o que importa: a
// finitude PRIMEIRO, porque toda comparação com NaN é falsa, e um cingir
// escripto na ordem natural (`m < 0 ? 0 : m > 1 ? 1 : m`) devolveria o NaN
// intacto ao floor, d'onde sahiria conta indefinida e indice fóra de limite.
float cingido(float magnitude) {
  if (!std::isfinite(magnitude)) return 0.0f;
  if (magnitude < 0.0f) return 0.0f;
  if (magnitude > 1.0f) return 1.0f;
  return magnitude;
}

}  // namespace

Registro registro_da_banda(float centro_em_hertz) {
  // A guarda da finitude vem PRIMEIRO, e pelo mesmo motivo que o cingido: toda
  // comparação com NaN é falsa, d'onde um NaN atravessaria os tres ramos e
  // sahiria AGUDOS, que é a familia que ninguem pediu. Sahe GRAVES, que é o
  // principio da escala e a côr que a obra já vestia.
  if (!std::isfinite(centro_em_hertz)) return Registro::Graves;
  if (centro_em_hertz <= FRONTEIRA_DOS_GRAVES) return Registro::Graves;
  if (centro_em_hertz <= FRONTEIRA_DOS_MEDIOS_GRAVES) return Registro::MediosGraves;
  if (centro_em_hertz <= FRONTEIRA_DOS_MEDIOS_AGUDOS) return Registro::MediosAgudos;
  return Registro::Agudos;
}

// O switch sem `default`, de proposito: registro novo accende aviso do
// compilador aqui e no nome, e o gate da issue #64 o converte em recusa. Com
// `default` o registro novo sahiria violeta e sem nome, calado. O return de
// baixo existe só porque a linguagem não sabe que o switch é exhaustivo.
std::string_view tinta_do_registro(Registro registro) {
  switch (registro) {
    case Registro::Graves: return tokens::v500;
    case Registro::MediosGraves: return tokens::data5;
    case Registro::MediosAgudos: return tokens::data3;
    case Registro::Agudos: return tokens::data2;
  }
  return tokens::v500;
}

std::string_view nome_do_registro(Registro registro) {
  switch (registro) {
    case Registro::Graves: return "GRAVES";
    case Registro::MediosGraves: return "MÉDIOS-GRAVES";
    case Registro::MediosAgudos: return "MÉDIOS-AGUDOS";
    case Registro::Agudos: return "AGUDOS";
  }
  return "GRAVES";
}

int oitavos(float magnitude, std::size_t altura) {
  const int teto = static_cast<int>(altura) * DEGRAUS_POR_CELULA;
  if (teto <= 0) return 0;
  // O floor, e não o arredondamento: enche-se o degrau que a magnitude JÁ
  // conquistou, e não o que ella quasi conquistou. D'onde a magnitude cheia dá o
  // teto EXACTO (floor de 1 * teto é teto) e o silencio dá zero exacto, que são
  // os dous extremos que o aceite cobra por nome.
  const float degraus = cingido(magnitude) * static_cast<float>(teto);
  const int conquistados = static_cast<int>(std::floor(degraus));
  return conquistados < 0 ? 0 : (conquistados > teto ? teto : conquistados);
}

std::string glifo_do_degrau(int degrau) {
  if (degrau <= 0) return std::string(kCelulaVazia);
  const int k = degrau > DEGRAUS_POR_CELULA ? DEGRAUS_POR_CELULA : degrau;
  // U+2580 + k, em UTF-8 de tres octetos. Escreve-se por ARITHMETICA do ponto
  // de codigo, e não por taboada de oito glifos crus, porque a taboada
  // permittiria um glifo fóra de ordem passar calado; a arithmetica não. O
  // terceiro octeto de U+2580 é 0x80, d'onde o de U+2580 + k é 0x80 + k, e k
  // vae de 1 a 8, que é U+2581 (um oitavo) a U+2588 (o bloco cheio).
  return std::string{'\xe2', '\x96', static_cast<char>('\x80' + k)};
}

std::vector<float> centros_das_bandas(
    const std::vector<std::size_t>& bordas_em_raias, float hertz_por_raia) {
  std::vector<float> centros;
  if (bordas_em_raias.size() < 2) return centros;
  centros.reserve(bordas_em_raias.size() - 1);
  for (std::size_t b = 0; b + 1 < bordas_em_raias.size(); ++b) {
    const float baixa = static_cast<float>(bordas_em_raias[b]) * hertz_por_raia;
    const float alta = static_cast<float>(bordas_em_raias[b + 1]) * hertz_por_raia;
    centros.push_back(baixa > 0.0f ? std::sqrt(baixa * alta) : 0.5f * alta);
  }
  return centros;
}

std::vector<float> centros_da_escala(std::size_t quantas) {
  std::vector<float> centros;
  if (quantas == 0) return centros;
  centros.reserve(quantas);
  const double razao = static_cast<double>(nucleo::HERTZ_MAXIMO) /
                       static_cast<double>(nucleo::HERTZ_MINIMO);
  for (std::size_t b = 0; b < quantas; ++b) {
    // A fracção do CENTRO é b mais meio sobre quantas, e não b sobre quantas,
    // que d'aquelle modo sahiria a borda de baixo em vez do meio da banda.
    const double parte =
        (static_cast<double>(b) + 0.5) / static_cast<double>(quantas);
    centros.push_back(
        static_cast<float>(nucleo::HERTZ_MINIMO * std::pow(razao, parte)));
  }
  return centros;
}

const Celula& Quadro::em(std::size_t linha, std::size_t collunha) const {
  // A célulla de fóra, uma só e immutavel: devolve-se referencia a ella em vez
  // de estourar, conforme o cabeçalho promette.
  static const Celula de_fora;
  if (linha >= altura || collunha >= largura) return de_fora;
  return celulas[linha * largura + collunha];
}

namespace {

// valor_da_columna — A REPARTIÇÃO, e é UMA funcção para os DOUS regimes.
//
// A collunha `c` de `largura` cobre o intervallo SEMI-ABERTO de bandas
// [c * n / largura, (c + 1) * n / largura), e toma o MÁXIMO d'ellas.
//
// Sendo a largura MENOR que n, o intervallo tem duas bandas ou mais, e o máximo
// FUNDE. Funde e não amostra, de propósito: amostrar faria um pico desapparecer
// só porque o operador estreitou a janella, e barra que apaga ao redimensionar
// lê-se como defeito. Sendo a largura MAIOR que n, o intervallo teria comprimento
// menor que um e sahiria VAZIO por truncamento; alarga-se ao minimo de uma banda,
// e então o máximo degenera em copia, que é o esticar.
//
// D'aqui sahe de graça o invariante que o aceite cobra: os intervallos partem
// [0, n) sem sobra e sem vão, d'onde banda alguma se perde em largura alguma.
//
// O INTERVALLO aparta-se em punho proprio porque DOUS leitores o querem: o valor
// da columna, que lhe toma o máximo, e o registro da columna, que lhe toma o
// meio. Escripta a conta duas vezes, um dia a côr apontaria para bandas que não
// são as que a barra mostra, e nada n'esta Casa o accusaria.
struct Intervallo {
  std::size_t principio = 0;
  std::size_t fim = 0;
};

Intervallo intervallo_da_columna(std::size_t quantas, std::size_t c,
                                 std::size_t largura) {
  Intervallo faixa;
  if (quantas == 0 || largura == 0) return faixa;
  faixa.principio = (c * quantas) / largura;
  if (faixa.principio >= quantas) faixa.principio = quantas - 1;
  faixa.fim = ((c + 1) * quantas) / largura;
  if (faixa.fim <= faixa.principio) faixa.fim = faixa.principio + 1;
  if (faixa.fim > quantas) faixa.fim = quantas;
  return faixa;
}

float valor_da_columna(const std::vector<float>& bandas, std::size_t c,
                       std::size_t largura) {
  const Intervallo faixa = intervallo_da_columna(bandas.size(), c, largura);
  float pico = 0.0f;
  for (std::size_t b = faixa.principio; b < faixa.fim; ++b)
    pico = std::max(pico, cingido(bandas[b]));
  return pico;
}

// registro_da_columna — o registro que veste a columna INTEIRA. Toma o centro da
// banda do MEIO do intervallo, e não o da banda que deu o pico: a côr é do
// LOGAR, e não do nivel, pelo mesmo motivo por que o invariante (iii) ancora o
// gradiente ao painel. Fosse do pico, a columna trocaria de côr a cada batida, e
// a legenda por baixo deixaria de dizer verdade.
Registro registro_da_columna(const std::vector<float>& centros, std::size_t c,
                             std::size_t largura) {
  const Intervallo faixa = intervallo_da_columna(centros.size(), c, largura);
  if (faixa.fim <= faixa.principio) return Registro::Graves;
  return registro_da_banda(
      centros[faixa.principio + (faixa.fim - faixa.principio - 1) / 2]);
}

}  // namespace

tokens::Triade tinta_da_linha(std::size_t desde_a_base, std::size_t altura,
                              Registro registro) {
  const std::string_view cor = tinta_do_registro(registro);
  // Painel de uma célulla só: a rampa degenera, e vale a BASE. A §7.4.9 ancora
  // a rampa na base, e painel de uma célulla é todo base; o meio da rampa seria
  // côr que spec alguma nomeia. E o desvio por zero fica excluido antes de se
  // chegar á divisão.
  if (altura <= 1) return tokens::mistura(cor, tokens::panel_hi, ALFA_DA_BASE);

  const std::size_t alto = desde_a_base >= altura ? altura - 1 : desde_a_base;
  const double t = static_cast<double>(alto) / static_cast<double>(altura - 1);

  // A interpolação vae por tokens::mistura, e NÃO por arithmetica de côr nova.
  // Ella compõe a frente sobre o fundo com o peso dado, d'onde t = 1 dá a côr do
  // registro EXACTA (peso cheio devolve a frente) e t = 0 dá a base EXACTA, sem
  // arredondamento a explicar. Uma segunda conta de côr abriria um segundo
  // caminho para o mesmo resultado, e dous caminhos divergem sem avisar.
  return tokens::mistura(cor, tokens::panel_hi,
                         ALFA_DA_BASE + (1.0 - ALFA_DA_BASE) * t);
}

namespace {

// tinta_da_celula — A PRECEDENCIA da côr, e a ordem É a regra. Lê-se de cima
// para baixo, e a primeira que responde ganha:
//   1. MUDO vence tudo, quente inclusive. Mudo é ordem do operador, e ordem do
//      operador não se deixa sobrepujar por leitura de sinal.
//   2. ZERO veste text_faint, que é o piso do silencio. Vem antes do quente por
//      pura arrumação (zero nunca é quente), e junto do mudo porque é a MESMA
//      côr que a §7.4.9 manda: mudo e silencio lêem-se egualmente apagados.
//   3. QUENTE veste glow_hot, e veste a COLUMNA INTEIRA. É a lógica do
//      bar_meter.lua, que faz `color = hot and glow_hot or FILL_COOL` e
//      substitue o enchimento todo, não sómente o cimo. Duas razões mais: só a
//      célulla do topo em glow_hot seria quasi invisivel n'uma fita que salta a
//      quarenta e seis quadros por segundo, que uma célulla a piscar não se lê;
//      e o indicador de pico existe para SER VISTO.
//   4. Não sendo nada d'isso, o GRADIENTE do painel, na côr do REGISTRO que
//      veste esta columna. O registro é da columna e não da célulla, d'onde a
//      columna inteira sahe da mesma familia, do pé ao topo.
// Note-se que sómente o ramo 4 consulta a linha, e sómente os ramos 1 a 3
// consultam o valor: nenhum consulta os dous, e é d'ahi que o gradiente não
// pode depender da magnitude nem por descuido.
tokens::Triade tinta_da_celula(float valor, bool mudo, std::size_t desde_a_base,
                               std::size_t altura, Registro registro) {
  if (mudo) return tokens::rgb(tokens::text_faint);
  if (valor <= 0.0f) return tokens::rgb(tokens::text_faint);
  if (valor >= LIMIAR_QUENTE) return tokens::rgb(tokens::glow_hot);
  return tinta_da_linha(desde_a_base, altura, registro);
}

}  // namespace

Quadro compor(const std::vector<float>& bandas, std::size_t largura,
              std::size_t altura, bool mudo,
              const std::vector<float>& centros_em_hertz) {
  Quadro quadro;
  quadro.largura = largura;
  quadro.altura = altura;
  if (largura == 0 || altura == 0) return quadro;  // painel sem célulla
  quadro.celulas.assign(largura * altura, Celula{});

  // Os deduzidos ficam n'um vector á parte, e a referencia elege qual vale: quem
  // passa os centros não paga copia alguma por quadro, e são quarenta e seis
  // quadros por segundo.
  std::vector<float> deduzidos;
  if (centros_em_hertz.empty()) deduzidos = centros_da_escala(bandas.size());
  const std::vector<float>& centros =
      centros_em_hertz.empty() ? deduzidos : centros_em_hertz;
  quadro.registros.assign(largura, Registro::Graves);

  for (std::size_t c = 0; c < largura; ++c) {
    quadro.registros[c] = registro_da_columna(centros, c, largura);
    const float valor = valor_da_columna(bandas, c, largura);
    const int degraus = oitavos(valor, altura);
    const std::size_t cheias =
        static_cast<std::size_t>(degraus / DEGRAUS_POR_CELULA);
    const int resto = degraus % DEGRAUS_POR_CELULA;

    // O PISO DO SILENCIO: barra em zero desenha UMA célulla de um oitavo, em vez
    // de nada. Para as barras «cahirem a zero e FICAREM em text_faint», como o
    // aceite pede, ellas precisam de continuar na tela: barra de zero célullas
    // não tem côr, e a promessa sahiria invacua. Um oitavo é o menor traço que o
    // terminal tem, e faz linha de base, que é o que um EQ mostra em silencio.
    const std::size_t desenhadas =
        degraus == 0 ? 1u : cheias + (resto > 0 ? 1u : 0u);

    for (std::size_t i = 0; i < desenhadas && i < altura; ++i) {
      const int degrau = degraus == 0
                             ? 1
                             : (i < cheias ? DEGRAUS_POR_CELULA : resto);
      Celula celula;
      celula.glifo = glifo_do_degrau(degrau);
      celula.tinta =
          tinta_da_celula(valor, mudo, i, altura, quadro.registros[c]);
      celula.pinta = true;
      // A INVERSÃO, e é a linha mais perigosa d'este manuscripto. `i` conta da
      // BASE para cima, que é como os blocos crescem; a linha do quadro conta do
      // TOPO para baixo, que é como o FTXUI pinta. D'onde a base é `altura - 1`.
      // Trocar isto por `i` desenha a fita de cabeça para baixo, e o defeito
      // passa em TODA prova de contagem, visto que o numero de célullas
      // desenhadas não muda. Por isso a prova o afirma por INDICE de linha.
      quadro.celulas[(altura - 1 - i) * largura + c] = std::move(celula);
    }
  }
  return quadro;
}

std::string sequencia_da_celula(const Celula& celula) {
  // A célulla que não pinta sahe em ORDEM DE REPOUSO, e não em tríade de côr
  // alguma: é a mesma regra que a fita arrowline segue com a côr transparente,
  // que no terminal não se pinta e vale por repouso.
  if (!celula.pinta) return std::string(tokens::repouso) + celula.glifo;
  // A tinta IMMEDIATAMENTE antes do glifo, sem repouso pelo meio. É o que fecha
  // a emenda entre célullas vizinhas de côres differentes: repouso intercalado
  // apagaria o fundo do painel entre uma barra e a seguinte.
  return tokens::sgr(38, celula.tinta) + celula.glifo;
}

ftxui::Element elemento_do_espectro(const Quadro& quadro) {
  std::vector<ftxui::Element> linhas;
  linhas.reserve(quadro.altura);

  for (std::size_t l = 0; l < quadro.altura; ++l) {
    std::vector<ftxui::Element> corridas;
    std::size_t c = 0;
    while (c < quadro.largura) {
      // Agrupa a CORRIDA de célullas de egual tinta n'um só elemento, em vez de
      // um elemento por célulla. Não é micro-optimização gratuita: o gradiente
      // ancorado no painel dá a MESMA tinta a toda a linha, d'onde a corrida
      // ordinaria é a linha inteira, e um painel de oitenta por doze cahe de
      // novecentos e sessenta elementos para doze.
      const Celula& cabeca = quadro.em(l, c);
      std::string texto;
      std::size_t fim = c;
      while (fim < quadro.largura) {
        const Celula& corrente = quadro.em(l, fim);
        if (corrente.pinta != cabeca.pinta) break;
        if (corrente.pinta && !mesma_tinta(corrente.tinta, cabeca.tinta)) break;
        texto += corrente.glifo;
        ++fim;
      }
      ftxui::Element pedaco = ftxui::text(texto);
      if (cabeca.pinta)
        pedaco = pedaco | ftxui::color(ftxui::Color::RGB(
                              cabeca.tinta.r, cabeca.tinta.g, cabeca.tinta.b));
      corridas.push_back(std::move(pedaco));
      c = fim;
    }
    linhas.push_back(ftxui::hbox(std::move(corridas)));
  }
  return ftxui::vbox(std::move(linhas));
}

}  // namespace mysong::tui

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
