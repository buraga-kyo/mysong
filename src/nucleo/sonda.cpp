// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA SONDA — src/nucleo/sonda.cpp
// ══════════════════════════════════════════════════════════════════════════
// A obra da sonda, em duas partes que não se misturam: a TABOA dos requisitos
// com a laçada que a percorre, que nada sabem do systema; e as tres consultas
// que interrogam o systema de verdade, que nada sabem da taboa.
//
// DOMÍNIO ......... a taboa d'este arquivo, e o inquerito que o chamador traz.
// CONTRA-DOMÍNIO .. o relatorio, com um estado por requisito da taboa.
// INVARIANTE ...... a taboa tem duração estática e chave UNICA por requisito;
//                   duas invocações de requisitos() devolvem a mesma referencia.
// Q.E.D. .......... requisito novo é LINHA nova na taboa, e nunca ramo novo na
//                   laçada; donde a sonda cresce sem que a laçada cresça.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/sonda.hpp"

#include <cstddef>

namespace mysong::nucleo {

// A TABOA. Os IMPEDIMENTOS primeiro, que é a ordem em que a tela os mostra: a
// fonte, sem a qual a estetica da casa vira quadrículo, e a libmpv, que é a
// machina de som e sem a qual não ha tocador algum, sómente moldura.
// O remedio cabe em UMA linha, e o mais remette-se ao README.
const std::vector<Requisito>& requisitos() {
  static const std::vector<Requisito> taboa = {
      {"fonte", "fonte com glifos de seta (Nerd Font)", Gravidade::Impedimento,
       Especie::FamiliaDeFonte, "nerd",
       "baixe uma Nerd Font de nerdfonts.com para ~/.local/share/fonts e rode "
       "fc-cache -fv"},
      {"libmpv", "libmpv (a machina de som)", Gravidade::Impedimento,
       Especie::Bibliotheca, "libmpv.so.2",
       "installe a libmpv: sudo apt install libmpv2 (ou libmpv-dev, para "
       "compilar)"},
      {"yt-dlp", "yt-dlp (o enchedor do acervo)", Gravidade::Aviso,
       Especie::Executavel, "yt-dlp",
       "installe o yt-dlp: pipx install yt-dlp (sem elle não se enche o "
       "acervo)"},
      {"chafa", "chafa (o mostrador de capa)", Gravidade::Aviso,
       Especie::Executavel, "chafa",
       "installe o chafa: sudo apt install chafa (sem elle não se vê capa)"},
  };
  return taboa;
}

// consultar — pergunta ao inquerito pela especie que o requisito declara. A
// consulta VAZIA responde ausente: inquerito mal montado accusa falta, e nunca
// dá por bom aquillo que não sabe.
namespace {
bool consultar(const Inquerito& inquerito, const Requisito& requisito) {
  const auto& quem = requisito.especie == Especie::FamiliaDeFonte
                         ? inquerito.familia_de_fonte
                     : requisito.especie == Especie::Bibliotheca
                         ? inquerito.bibliotheca
                         : inquerito.executavel;
  return quem ? quem(requisito.alvo) : false;
}
}  // namespace

// sondar — a laçada. Uma pergunta por requisito da taboa, na ordem d'ella, e
// nada mais: não pinta, não escreve, não lê ambiente, não sahe do programa.
Relatorio sondar(const Inquerito& inquerito) {
  Relatorio relatorio;
  relatorio.estados.reserve(requisitos().size());
  for (const Requisito& requisito : requisitos())
    relatorio.estados.push_back({requisito, consultar(inquerito, requisito)});
  return relatorio;
}

bool Relatorio::ha_falta() const {
  for (const Estado& estado : estados)
    if (!estado.presente) return true;
  return false;
}

// ha_impedimento — a pergunta que decide se o programa abre. Aviso não conta
// aqui, e impedimento conta ainda que venha acompanhado de avisos.
bool Relatorio::ha_impedimento() const {
  for (const Estado& estado : estados)
    if (!estado.presente &&
        estado.requisito.gravidade == Gravidade::Impedimento)
      return true;
  return false;
}

// faltas — os impedimentos adeante dos avisos, cada grupo na ordem da taboa.
// Duas passagens, e não ordenação: ordenar pediria comparador estavel para dar
// exactamente este resultado, por mais palavras e menos evidencia.
std::vector<Estado> Relatorio::faltas() const {
  std::vector<Estado> colhidas;
  for (const Gravidade gravidade : {Gravidade::Impedimento, Gravidade::Aviso})
    for (const Estado& estado : estados)
      if (!estado.presente && estado.requisito.gravidade == gravidade)
        colhidas.push_back(estado);
  return colhidas;
}

namespace {

// contem_insensivel — busca a agulha no palheiro, cega á caixa em ASCII. Faz-se
// á mão, e não por std::tolower, porque aquelle depende do locale corrente: sob
// locale turco o 'I' desce a caractere que não é 'i', e a familia "NERD" fugiria
// da busca por razão que ninguem havia de suspeitar. Nome de familia de fonte é
// ASCII no que nos importa, e a caixa é a unica variação que se tolera.
bool contem_insensivel(std::string_view palheiro, std::string_view agulha) {
  const auto baixa = [](char letra) {
    return letra >= 'A' && letra <= 'Z' ? static_cast<char>(letra | 0x20)
                                        : letra;
  };
  if (agulha.empty() || agulha.size() > palheiro.size()) return false;
  for (std::size_t inicio = 0; inicio + agulha.size() <= palheiro.size();
       ++inicio) {
    std::size_t passo = 0;
    while (passo < agulha.size() &&
           baixa(palheiro[inicio + passo]) == baixa(agulha[passo]))
      ++passo;
    if (passo == agulha.size()) return true;
  }
  return false;
}

}  // namespace

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
