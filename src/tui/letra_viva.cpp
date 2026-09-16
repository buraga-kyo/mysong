// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LETRA VIVA, src/tui/letra_viva.cpp
// ══════════════════════════════════════════════════════════════════════════
// A lavra do que src/tui/letra_viva.hpp declara. O contracto, o dominio e os
// invariantes moram lá, e não se repetem aqui.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/letra_viva.hpp"

#include <cmath>
#include <cstdint>
#include <utility>

namespace mysong::tui {
namespace {

// cingido, o valor no intervallo fechado. Mora aqui, e não em tokens: aquelle
// cinge alfa, e alfa é caso d'este, e não o contrario.
double cingido(double valor, double baixo, double alto) {
  return valor < baixo ? baixo : (valor > alto ? alto : valor);
}

// e_branco, o glypho que o embaralho CONSERVA. É o espaço, e é elle que deixa
// a fórma das palavras a ler-se antes de as letras se resolverem: mexido, a
// linha viraria uma barra de lixo e o olho não veria verso nenhum a chegar.
bool e_branco(const std::string& glifo) { return glifo == " "; }

}  // namespace

std::size_t linha_de_leitura(std::size_t altura) {
  return altura == 0 ? 0 : altura / 3;
}

double nascimento_da_linha(const std::vector<nucleo::LinhaDaLetra>& linhas,
                           std::size_t qual) {
  if (qual >= linhas.size()) return NASCIMENTO_MAXIMO;
  const double anterior = qual == 0 ? 0.0 : linhas[qual - 1].tempo;
  return cingido(linhas[qual].tempo - anterior, NASCIMENTO_MINIMO,
                 NASCIMENTO_MAXIMO);
}

std::vector<std::string> glifos_da_linha(std::string_view texto) {
  std::vector<std::string> saida;
  for (std::size_t i = 0; i < texto.size();) {
    const unsigned char oct = static_cast<unsigned char>(texto[i]);
    // A continuação SOLTA (o octeto 10xxxxxx sem cabeça) vale por um glypho de
    // um octeto: cadeia mal fórmada não ha de fazer o laço andar para traz nem
    // ler fóra do fim, e letra que veio rota mostra-se rota.
    std::size_t quantos = 1;
    if (oct >= 0xf0) quantos = 4;
    else if (oct >= 0xe0) quantos = 3;
    else if (oct >= 0xc0) quantos = 2;
    if (i + quantos > texto.size()) quantos = 1;
    saida.emplace_back(texto.substr(i, quantos));
    i += quantos;
  }
  return saida;
}

namespace {

// mistura_do_acaso, a mistura de bits do splitmix64. Gerador do systema NÃO ha
// n'esta obra, e por isso se escreve: `rand()` daria fita differente a cada
// corrida, e prova alguma se poderia fazer d'ella. Da mesma semente sahe sempre
// o mesmo numero, e a semente é a POSIÇÃO, que é o que a pureza exige.
std::uint64_t mistura_do_acaso(std::uint64_t semente) {
  std::uint64_t x = semente + 0x9e3779b97f4a7c15ull;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
  return x ^ (x >> 31);
}

}  // namespace

std::string embaralha(const std::vector<std::string>& glifos, double resolvida,
                      std::size_t qual, long long quadro) {
  const std::size_t quantos = glifos.size();
  if (quantos == 0) return {};
  // A FONTE do embaralho são as proprias letras da linha, e sómente ellas: sahe
  // fita que se lê como a mesma lingua, com os mesmos acentos, e não ruido.
  std::vector<std::size_t> fonte;
  for (std::size_t k = 0; k < quantos; ++k)
    if (!e_branco(glifos[k])) fonte.push_back(k);

  // Os RESOLVIDOS são os do MEIO: a janella abre-se do centro para as pontas, e
  // é d'ahi que sahe o «ganhar fórma» que a issue pede. Contada das pontas para
  // o meio, a linha resolver-se-hia pelo fim, que é onde o olho não a lê.
  const std::size_t resolvidos = static_cast<std::size_t>(std::llround(
      cingido(resolvida, 0.0, 1.0) * static_cast<double>(quantos)));
  const std::size_t inicio = (quantos - resolvidos) / 2;

  std::string saida;
  for (std::size_t k = 0; k < quantos; ++k) {
    if ((k >= inicio && k < inicio + resolvidos) || fonte.empty() ||
        e_branco(glifos[k])) {
      saida += glifos[k];
      continue;
    }
    const std::uint64_t semente =
        static_cast<std::uint64_t>(qual) * 1000003ull +
        static_cast<std::uint64_t>(quadro) * 8191ull + k;
    saida += glifos[fonte[mistura_do_acaso(semente) % fonte.size()]];
  }
  return saida;
}

namespace {

// TINTA DA SUBIDA. Tres terços, e tres tokens, como a issue os nomeia: o
// apagado ao nascer, o corpo a meia subida, o brilhante no ultimo terço, que é
// quando o olho já a lê inteira e a linha está a chegar.
std::string_view tinta_da_subida(double fracao) {
  if (fracao < 1.0 / 3.0) return tokens::text_faint;
  if (fracao < 2.0 / 3.0) return tokens::text_body;
  return tokens::text_bright;
}

// posta_na_tela, o texto da linha, cortado á largura e CENTRADO. O corte remata
// em reticencias, e a collunha é a que sobra repartida em duas: linha comprida
// não empurra as outras nem sahe do painel.
void posta_na_tela(const std::string& texto, std::size_t largura,
                   std::size_t qual, long long quadro, LinhaViva* viva) {
  std::vector<std::string> glifos = glifos_da_linha(texto);
  if (glifos.size() > largura) {
    glifos.resize(largura - 1);
    glifos.emplace_back("…");
  }
  viva->collunha = (largura - glifos.size()) / 2;
  // O VERSO sahe do MESMO corte, e não de um segundo: cortado outra vez adeante,
  // a chapa em XIROD poderia dizer o que a linha em mono não diz.
  for (const std::string& glifo : glifos) viva->verso += glifo;
  viva->texto = embaralha(glifos, viva->resolvida, qual, quadro);
}

}  // namespace

QuadroDaLetra quadro_da_letra(const std::vector<nucleo::LinhaDaLetra>& linhas,
                              double posicao, std::size_t largura,
                              std::size_t altura) {
  QuadroDaLetra quadro;
  quadro.largura = largura;
  quadro.altura = altura;
  if (linhas.empty() || largura == 0 || altura == 0) return quadro;

  const std::size_t leitura = linha_de_leitura(altura);
  const std::size_t base = altura - 1;
  // A conta do embaralho sahe da POSIÇÃO, e por isso o quadro continua puro: o
  // fervilhar anda com a musica, e parado com ella.
  const long long conta = static_cast<long long>(
      std::floor(posicao * static_cast<double>(QUADROS_DO_EMBARALHO)));

  for (std::size_t i = 0; i < linhas.size(); ++i) {
    const double instante = linhas[i].tempo;
    const double nasce = nascimento_da_linha(linhas, i);
    if (posicao < instante - nasce) continue;  // ainda não assomou
    const bool ha_proxima = i + 1 < linhas.size();
    const double proximo = ha_proxima ? linhas[i + 1].tempo : 0.0;

    LinhaViva viva;
    viva.qual = i;
    if (posicao < instante) {
      // A SUBIDA, linear da base á leitura. Chega ao topo do vão no instante
      // exacto: é o que faz a linha assentar quando a voz a canta.
      const double sobe =
          cingido((posicao - (instante - nasce)) / nasce, 0.0, 1.0);
      const std::size_t vao = base - leitura;
      viva.linha_da_tela =
          base - static_cast<std::size_t>(
                     std::llround(sobe * static_cast<double>(vao)));
      viva.resolvida = sobe;
      viva.tinta = tinta_da_subida(sobe);
      viva.sobe = true;
    } else if (!ha_proxima || posicao < proximo) {
      viva.linha_da_tela = leitura;
      viva.resolvida = 1.0;
      viva.tinta = tokens::text_bright;
      viva.corrente = true;
    } else {
      // O APAGAR. Uma linha por segundo, a contar do instante da SEGUINTE: é
      // ella que toma a linha de leitura, e duas na mesma linha não cabem.
      const double subidas = std::floor(posicao - proximo) + 1.0;
      if (subidas > static_cast<double>(leitura)) continue;  // morreu no alto
      const std::size_t degraus = static_cast<std::size_t>(subidas);
      viva.linha_da_tela = leitura - degraus;
      viva.resolvida = 1.0;
      viva.tinta = degraus == 1 ? tokens::text_muted : tokens::text_faint;
    }

    if (linhas[i].texto.empty()) continue;  // o silencio marcado não pinta
    posta_na_tela(linhas[i].texto, largura, i, conta, &viva);
    if (viva.corrente) quadro.corrente = static_cast<int>(quadro.linhas.size());
    quadro.linhas.push_back(std::move(viva));
  }
  return quadro;
}

const LinhaViva* linha_corrente_do_rio(const QuadroDaLetra& quadro) {
  if (quadro.corrente < 0) return nullptr;
  const std::size_t qual = static_cast<std::size_t>(quadro.corrente);
  return qual < quadro.linhas.size() ? &quadro.linhas[qual] : nullptr;
}

const LinhaViva* linha_que_sobe_do_rio(const QuadroDaLetra& quadro) {
  // A PRIMEIRA que sobe, e não a ultima: sendo uma só, as duas seriam a mesma,
  // e parando na primeira o laço não percorre o rio inteiro por nada.
  for (const LinhaViva& viva : quadro.linhas)
    if (viva.sobe) return &viva;
  return nullptr;
}

Rectangulo caixa_da_corrente(const QuadroDaLetra& quadro) {
  const LinhaViva* viva = linha_corrente_do_rio(quadro);
  if (viva == nullptr) return {};
  return {viva->collunha, viva->linha_da_tela,
          glifos_da_linha(viva->texto).size(), 1};
}

ChapaDaLetra ordem_da_chapa_da_letra(const QuadroDaLetra& rio,
                                     const Rectangulo& espectro,
                                     bool letreiro_de_pe, bool foco_dentro,
                                     bool mostra_letra) {
  ChapaDaLetra ordem;
  // As quatro condições são de CONJUNCÇÃO, e nenhuma sobra: sem letreiro não ha
  // chapa que pôr, o foco fóra manda tirar, o `l` escondido tambem, e painel
  // que se não pinta não tem canto onde a janella assente.
  if (!letreiro_de_pe || !foco_dentro || !mostra_letra || espectro.vazio())
    return ordem;
  const LinhaViva* corrente = linha_corrente_do_rio(rio);
  const Rectangulo caixa = caixa_da_corrente(rio);
  if (corrente != nullptr && !caixa.vazio()) {
    ordem.poe = true;
    // O canto do PAINEL sommado á caixa do rio: aquelle é o unico que sabe onde
    // o painel começa, e esta o unico que sabe onde o verso assenta.
    ordem.collunha = static_cast<int>(espectro.x + caixa.x);
    ordem.linha = static_cast<int>(espectro.y + caixa.y);
    ordem.cellulas = caixa.largura;
    ordem.verso = corrente->verso;
  }
  // A que SOBE adianta-se ainda que corrente alguma haja: é o caso da primeira
  // linha da faixa, que nasce sem quem a preceda na leitura, e é justamente
  // essa que não ha de esperar pelo pango-view.
  const LinhaViva* proxima = linha_que_sobe_do_rio(rio);
  if (proxima != nullptr) {
    ordem.adiantado = proxima->verso;
    ordem.cellulas_adiantadas = glifos_da_linha(proxima->verso).size();
  }
  return ordem;
}

nucleo::PedidoDaChapa pedido_da_chapa_da_letra(const std::string& verso,
                                               std::size_t cellulas) {
  nucleo::PedidoDaChapa pedido;
  pedido.texto = verso;
  // O BRILHO CHEIO da linha de leitura, e o fundo do painel por cama: são as
  // duas côres com que a linha em mono já se pinta debaixo d'ella, e é d'essa
  // egualdade que a imagem assenta sem se ver emenda.
  // LARANJA (issue #157), que foi o que elle pediu: o `data3` da paleta, que é
  // o laranja do poente do RADICAL-OS. Fundo do painel, que é a cella por baixo.
  // E a familia da LEITURA (issue #159), e não a da marca: verso inteiro em
  // XIROD custa a ler, e elle disse-o ao vê-lo na tela.
  pedido.familia = std::string(nucleo::FAMILIA_DA_LEITURA);
  pedido.tinta = std::string(tokens::data3);
  pedido.fundo = std::string(tokens::panel);
  pedido.cellulas = cellulas;
  // TRES fileiras de cella, que é o corpo GRANDE d'ella: a sala reserva-as, e é
  // d'aqui que sae o tamanho da lettra.
  pedido.linhas = FILEIRAS_DO_VERSO;
  pedido.corpo = nucleo::corpo_da_altura(FILEIRAS_DO_VERSO);
  return pedido;
}

std::vector<CelulaDoRio> tapete_do_rio(const Quadro& espectro,
                                       const QuadroDaLetra& letra) {
  std::vector<CelulaDoRio> tapete(espectro.largura * espectro.altura);
  // O ESPECTRO primeiro, inteiro: o rio não o apaga, cobre-o.
  for (std::size_t l = 0; l < espectro.altura; ++l)
    for (std::size_t c = 0; c < espectro.largura; ++c) {
      const Celula& d_elle = espectro.em(l, c);
      CelulaDoRio& n_ella = tapete[l * espectro.largura + c];
      n_ella.glifo = d_elle.glifo;
      n_ella.tinta = d_elle.tinta;
      n_ella.pinta = d_elle.pinta;
    }
  // A LETRA por cima, e na ordem em que as linhas vêm: a de indice maior é a
  // mais nova, e duas que cahiam na mesma linha da tela hão de deixar ver a que
  // está a chegar, e não a que já se foi.
  for (const LinhaViva& viva : letra.linhas) {
    if (viva.linha_da_tela >= espectro.altura) continue;
    std::size_t c = viva.collunha;
    for (const std::string& glifo : glifos_da_linha(viva.texto)) {
      if (c >= espectro.largura) break;
      if (!e_branco(glifo)) {
        CelulaDoRio& n_ella = tapete[viva.linha_da_tela * espectro.largura + c];
        n_ella.glifo = glifo;
        n_ella.tinta = tokens::rgb(viva.tinta);
        n_ella.pinta = true;
        n_ella.letra = true;
      }
      ++c;
    }
  }
  return tapete;
}

std::string sequencia_do_rio(const CelulaDoRio& celula) {
  std::string bytes;
  if (celula.letra) bytes += tokens::fundo_de(tokens::panel);
  bytes += celula.pinta ? tokens::sgr(38, celula.tinta)
                        : std::string(tokens::repouso);
  return bytes + celula.glifo;
}

ftxui::Element elemento_do_rio(const Quadro& espectro,
                               const QuadroDaLetra& letra) {
  const std::vector<CelulaDoRio> tapete = tapete_do_rio(espectro, letra);
  const tokens::Triade fundo = tokens::rgb(tokens::panel);
  std::vector<ftxui::Element> pintadas;
  pintadas.reserve(espectro.altura);

  for (std::size_t l = 0; l < espectro.altura; ++l) {
    std::vector<ftxui::Element> corridas;
    std::size_t c = 0;
    while (c < espectro.largura) {
      // A CORRIDA de célullas de egual vestido n'um só elemento, como o
      // espectro já o fazia: o gradiente d'elle é ancorado ao painel.
      const CelulaDoRio& cabeca = tapete[l * espectro.largura + c];
      std::string texto;
      std::size_t fim = c;
      while (fim < espectro.largura) {
        const CelulaDoRio& corrente = tapete[l * espectro.largura + fim];
        if (corrente.pinta != cabeca.pinta) break;
        if (corrente.letra != cabeca.letra) break;
        if (corrente.pinta && !mesma_tinta(corrente.tinta, cabeca.tinta)) break;
        texto += corrente.glifo;
        ++fim;
      }
      ftxui::Element pedaco = ftxui::text(texto);
      if (cabeca.pinta)
        pedaco = std::move(pedaco) | ftxui::color(ftxui::Color::RGB(
                                         cabeca.tinta.r, cabeca.tinta.g,
                                         cabeca.tinta.b));
      // O FUNDO DO PAINEL sómente debaixo da letra: esconde a barra de UMA
      // célulla e deixa as vizinhas a mexer.
      if (cabeca.letra)
        pedaco = std::move(pedaco) | ftxui::bgcolor(ftxui::Color::RGB(
                                         fundo.r, fundo.g, fundo.b));
      corridas.push_back(std::move(pedaco));
      c = fim;
    }
    pintadas.push_back(ftxui::hbox(std::move(corridas)));
  }
  return ftxui::vbox(std::move(pintadas));
}

namespace {

// aparado, o verso cortado á largura, com «…» a fechar, medido em COLLUNHAS do
// terminal: ha letra com kanji e com emoji, e aquellas valem duas.
std::string aparado(const std::string& verso, std::size_t largura) {
  if (largura == 0) return {};
  if (static_cast<std::size_t>(ftxui::string_width(verso)) <= largura)
    return verso;
  std::string feito;
  std::size_t gastas = 0;
  for (std::size_t i = 0; i < verso.size();) {
    std::size_t fim = i + 1;
    while (fim < verso.size() &&
           (static_cast<unsigned char>(verso[fim]) & 0xC0) == 0x80)
      ++fim;
    const std::string letra = verso.substr(i, fim - i);
    const std::size_t vale =
        static_cast<std::size_t>(ftxui::string_width(letra));
    if (gastas + vale > largura - 1) break;
    feito += letra;
    gastas += vale;
    i = fim;
  }
  return feito + "\u2026";
}

// ao_centro, o verso centrado na largura, com o fundo do painel de um lado ao
// outro: verso encostado á esquerda leria-se como lista, e isto não é lista.
ftxui::Element ao_centro(const std::string& verso, std::string_view tinta,
                         std::size_t largura, bool forte) {
  const std::string cortado = aparado(verso, largura);
  const std::size_t mede =
      static_cast<std::size_t>(ftxui::string_width(cortado));
  const std::size_t antes = mede < largura ? (largura - mede) / 2 : 0;
  const std::size_t depois =
      mede + antes < largura ? largura - mede - antes : 0;
  const tokens::Triade c = tokens::rgb(tinta);
  ftxui::Element feito =
      ftxui::text(std::string(antes, ' ') + cortado + std::string(depois, ' ')) |
      ftxui::color(ftxui::Color::RGB(c.r, c.g, c.b));
  return forte ? std::move(feito) | ftxui::bold : feito;
}

// verso_de, o texto de um indice que pode não existir. Fóra da letra dá vazio,
// e a fileira sahe em branco: é o que se vê no principio e no fim da musica.
std::string verso_de(const std::vector<nucleo::LinhaDaLetra>& linhas, int qual) {
  if (qual < 0 || static_cast<std::size_t>(qual) >= linhas.size()) return {};
  return linhas[static_cast<std::size_t>(qual)].texto;
}

}  // namespace

AssignaturaDaChapa assignatura_da(const ChapaDaLetra& ordem) {
  return {ordem.verso, ordem.collunha, ordem.linha, ordem.cellulas};
}

bool limpa_antes_de_por(const AssignaturaDaChapa& posta,
                        const AssignaturaDaChapa& nova, bool ha_posta) noexcept {
  if (!ha_posta) return false;   // nada ha na tela que limpar
  return !(posta == nova);       // a mesma não se limpa: piscaria por quadro
}

std::string verso_do_bloco(const std::vector<nucleo::LinhaDaLetra>& linhas,
                           int corrente, std::size_t largura) {
  if (linhas.empty() || largura == 0) return {};
  // ANTES do primeiro verso mostra-se o PRIMEIRO, que é o que vem a caminho: o
  // bloco em branco no principio da musica leria-se como faixa sem letra.
  const int qual = corrente < 0 ? 0 : corrente;
  return aparado(verso_de(linhas, qual), largura);
}

ftxui::Element elemento_da_letra_parada(
    const std::vector<nucleo::LinhaDaLetra>& linhas, int corrente,
    std::size_t largura, std::size_t altura, bool pela_chapa) {
  if (largura == 0 || altura == 0) return ftxui::emptyElement();
  const int qual = corrente < 0 ? 0 : corrente;
  std::vector<ftxui::Element> fileiras;
  fileiras.reserve(altura);
  for (std::size_t f = 0; f < altura; ++f) {
    // O CORRENTE no alto, GRANDE e laranja: elle assenta na primeira fileira, e
    // as duas seguintes ficam em branco, que são a caixa que a chapa cobre.
    if (f == FILEIRA_DO_CORRENTE && !linhas.empty()) {
      // Com a CHAPA de pé (issue #165), a cella pinta sómente o fundo: a
      // imagem cobre-a, e o mono por baixo appareceria de fóra d'ella, que
      // ella cabe por altura e sahe mais curta que o texto.
      fileiras.push_back(ao_centro(
          pela_chapa ? std::string() : verso_de(linhas, qual), tokens::data3,
          largura, !pela_chapa));
      continue;
    }
    // O SEGUINTE na ultima (issue #159), miudo e apagado: elle diz o que vem, e
    // apagado de proposito, que dous versos accesos disputariam o olho. Sendo o
    // corrente o ultimo da letra, a fileira fica VAZIA: recado algum se inventa.
    if (f == FILEIRA_DO_SEGUINTE && f != FILEIRA_DO_CORRENTE)
      fileiras.push_back(ao_centro(verso_de(linhas, qual + 1), tokens::text_muted,
                                   largura, false));
    else
      fileiras.push_back(ao_centro({}, tokens::text_muted, largura, false));
  }
  return ftxui::vbox(std::move(fileiras)) |
         ftxui::size(ftxui::WIDTH, ftxui::EQUAL, static_cast<int>(largura)) |
         ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, static_cast<int>(altura));
}

ChapaDaLetra ordem_da_chapa_parada(const std::vector<nucleo::LinhaDaLetra>& linhas,
                                   int corrente, const ftxui::Box& caixa,
                                   bool letreiro_de_pe, bool foco_dentro,
                                   bool mostra_letra) {
  ChapaDaLetra ordem;
  if (!letreiro_de_pe || !foco_dentro || !mostra_letra || caixa.IsEmpty())
    return ordem;
  const std::size_t largura =
      static_cast<std::size_t>(caixa.x_max - caixa.x_min + 1);
  const std::string verso = verso_do_bloco(linhas, corrente, largura);
  if (verso.empty()) return ordem;
  const std::size_t mede =
      static_cast<std::size_t>(ftxui::string_width(verso));
  ordem.poe = true;
  ordem.cellulas = mede;
  ordem.collunha =
      caixa.x_min +
      static_cast<int>(largura > mede ? (largura - mede) / 2 : 0);
  ordem.linha = caixa.y_min + static_cast<int>(FILEIRA_DO_CORRENTE);
  ordem.verso = verso;
  return ordem;
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO., buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
