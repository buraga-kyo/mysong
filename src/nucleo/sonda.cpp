// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA SONDA, src/nucleo/sonda.cpp
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
#include <cstdlib>
#include <string>

#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fontconfig/fontconfig.h>

namespace mysong::nucleo {

// A TABOA. Os IMPEDIMENTOS primeiro, que é a ordem em que a tela os mostra: a
// fonte, sem a qual a estetica da casa vira quadrículo, e a libmpv, que é a
// machina de som e sem a qual não ha tocador algum, sómente moldura.
// O remedio cabe em UMA linha, e o mais remette-se ao README.
const std::vector<Requisito>& requisitos() {
  static const std::vector<Requisito> taboa = {
      {"fonte", "fonte com glifos de seta (Nerd Font)", Gravidade::Impedimento,
       Especie::FamiliaDeFonte, CLASSE_DA_FONTE,
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
      {"ffmpeg", "ffmpeg (o desenhador da onda)", Gravidade::Aviso,
       Especie::Executavel, "ffmpeg",
       "installe o ffmpeg: sudo apt install ffmpeg (sem elle a fita mostra a "
       "barra chata em logar da onda)"},
  };
  return taboa;
}

// consultar, pergunta ao inquerito pela especie que o requisito declara. A
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

// sondar, a laçada. Uma pergunta por requisito da taboa, na ordem d'ella, e
// nada mais: não pinta, não escreve, não lê ambiente, não sahe do programa.
Relatorio sondar(const Inquerito& inquerito) {
  Relatorio relatorio;
  relatorio.estados.reserve(requisitos().size());
  for (const Requisito& requisito : requisitos())
    relatorio.estados.push_back({requisito, consultar(inquerito, requisito)});
  return relatorio;
}

bool Relatorio::ha_falta() const {
  for (const Veredicto& estado : estados)
    if (!estado.presente) return true;
  return false;
}

// ha_impedimento, a pergunta que decide se o programa abre. Aviso não conta
// aqui, e impedimento conta ainda que venha acompanhado de avisos.
bool Relatorio::ha_impedimento() const {
  for (const Veredicto& estado : estados)
    if (!estado.presente &&
        estado.requisito.gravidade == Gravidade::Impedimento)
      return true;
  return false;
}

// faltas, os impedimentos adeante dos avisos, cada grupo na ordem da taboa.
// Duas passagens, e não ordenação: ordenar pediria comparador estavel para dar
// exactamente este resultado, por mais palavras e menos evidencia.
std::vector<Veredicto> Relatorio::faltas() const {
  std::vector<Veredicto> colhidas;
  for (const Gravidade gravidade : {Gravidade::Impedimento, Gravidade::Aviso})
    for (const Veredicto& estado : estados)
      if (!estado.presente && estado.requisito.gravidade == gravidade)
        colhidas.push_back(estado);
  return colhidas;
}

namespace {

// contem_insensivel, busca a agulha no palheiro, cega á caixa em ASCII. Faz-se
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

// ha_familia_de_fonte, pergunta ao fontconfig se alguma familia installada
// traz a agulha no nome.
//
// DO LIMITE, que se declara aqui e se repete na tela: o fontconfig sabe o que
// está installado no SYSTEMA, e não o que o emulador de terminal elegeu. Essa
// segunda consulta NÃO EXISTE: nenhum programa que corra dentro do terminal
// alcança a fonte que o emulador escolheu. Apanha-se pois o caso commum, que é
// a fonte ausente, e fica de fóra o caso da fonte presente com o terminal
// apontado a outra. Heuristica por largura de glifo seria falso negativo pior
// que a lacuna, e por isso se recusa.
//
// Fontconfig que não inicialize conta-se FALTA, e nunca presença: o silencio
// d'elle é ignorancia nossa, e ignorancia não se resolve por optimismo.
//
// O ceremonial (abrir a configuração, armar o padrão, listar, e desfazer os
// quatro punhos) mora AQUI e n'um logar só, e o que varia entra por `basta`:
// pergunta nova sobre as fontes é predicado novo, e não copia d'este bloco.
// Pára na primeira familia que responda verdadeiro.
bool alguma_familia(std::string_view agulha,
                    const std::function<bool(FcPattern*)>& basta) {
  // Agulha vazia sahe ANTES de se abrir cousa alguma: o contem_insensivel já
  // lhe responderia não, e passear pelas fontes todas para o ouvir é gasto.
  if (agulha.empty()) return false;
  FcConfig* configuracao = FcInitLoadConfigAndFonts();
  if (configuracao == nullptr) return false;
  FcPattern* padrao = FcPatternCreate();
  FcObjectSet* campos = FcObjectSetBuild(FC_FAMILY, FC_CHARSET, nullptr);
  FcFontSet* achadas = (padrao != nullptr && campos != nullptr)
                           ? FcFontList(configuracao, padrao, campos)
                           : nullptr;
  bool achou = false;
  for (int posicao = 0;
       achadas != nullptr && posicao < achadas->nfont && !achou; ++posicao) {
    FcChar8* familia = nullptr;
    if (FcPatternGetString(achadas->fonts[posicao], FC_FAMILY, 0, &familia) ==
            FcResultMatch &&
        familia != nullptr &&
        contem_insensivel(reinterpret_cast<const char*>(familia), agulha))
      achou = basta(achadas->fonts[posicao]);
  }
  if (achadas != nullptr) FcFontSetDestroy(achadas);
  if (campos != nullptr) FcObjectSetDestroy(campos);
  if (padrao != nullptr) FcPatternDestroy(padrao);
  FcConfigDestroy(configuracao);
  return achou;
}

// ha_familia_de_fonte, o nome basta, e nada mais se pergunta á fonte.
bool ha_familia_de_fonte(std::string_view agulha) {
  return alguma_familia(agulha, [](FcPattern*) { return true; });
}

// ha_bibliotheca, tenta CARREGAR a bibliotheca pelo seu soname, e logo a
// solta. Perguntar por dlopen devolve a ausencia como RESPOSTA, e não como morte
// antes da primeira linha.
//
// E isto descreve o binario que EXISTE, e não uma intenção: desde a issue #28,
// binario algum d'esta obra liga a libmpv em tempo de ligação. Nem o mysong, nem
// o toca_tom, nem os demais exemplos. O motor chama-a pela taboa de
// nucleo/libmpv.hpp, aberta tambem por dlopen, e o CMakeLists pede d'ella
// sómente os cabeçalhos.
//
// Antes d'aquella issue a garantia era ACCIDENTAL, e vale registrar o mal: o
// mysong não ligava a libmpv sómente porque a tela ainda não chamava o motor, e
// o ligador não puxa de bibliotheca estatica o objecto que ninguem usa. No dia
// em que a tela o chamasse, o processo morreria no carregador dynamico antes do
// main, e esta sonda nunca correria para nomear a falta. Quem guarda a promessa
// hoje é a prova da ligação, em testes/prova_libmpv.cpp: ella lê o binario
// produzido e falha se a libmpv voltar a ser dependencia de ligação.
//
// Acha-se a runtime, e não os cabeçalhos de compilação: para um binario já
// compilado, que é o caso, a runtime é o que importa.
//
// Solta-se aqui, e a taboa do motor NÃO solta: esta nada guarda da bibliotheca,
// e aquella guarda punho vivo, que dlclose derrubaria.
bool ha_bibliotheca(std::string_view soname) {
  const std::string nome(soname);
  void* punho = dlopen(nome.c_str(), RTLD_LAZY | RTLD_LOCAL);
  if (punho == nullptr) return false;
  dlclose(punho);
  return true;
}

// ha_executavel, procura o nome nos directorios do PATH e exige permissão de
// EXECUÇÃO, e não mera existencia: arquivo que está lá e não corre é ausente
// para quem precisa correr, e directorio homonymo tambem o é, dahi a exigencia
// de arquivo regular. PATH ausente do ambiente trata-se como cadeia vazia; e
// componente VAZIO, que é o "::" do meio, não se lê como directorio corrente,
// que seria porta aberta a mau costume.
bool ha_executavel(std::string_view nome) {
  const char* const caminho = std::getenv("PATH");
  if (caminho == nullptr) return false;
  std::string_view resto(caminho);
  while (!resto.empty()) {
    const std::size_t corte = resto.find(':');
    const std::string_view pasta = resto.substr(
        0, corte == std::string_view::npos ? resto.size() : corte);
    if (!pasta.empty()) {
      std::string tentativa(pasta);
      tentativa += '/';
      tentativa.append(nome);
      struct stat marca = {};
      if (::stat(tentativa.c_str(), &marca) == 0 && S_ISREG(marca.st_mode) &&
          ::access(tentativa.c_str(), X_OK) == 0)
        return true;
    }
    if (corte == std::string_view::npos) break;
    resto.remove_prefix(corte + 1);
  }
  return false;
}

// A FORÇAGEM. Sem ella o caminho do impedimento não se demonstraria á mão nesta
// machina, onde os quatro requisitos estão presentes: haveria de se desinstalar
// a libmpv para ver a tela, o que ninguem faz por gosto. Nomeando-se as chaves
// em MYSONG_SONDA_FORCA, o binario finge a ausencia sem que o systema se mexa.
//
// É o UNICO logar d'esta obra que lê variavel de ambiente, e fica de proposito
// fóra de sondar(), que se conserva pura. Chave desconhecida ignora-se em
// silencio: erro de dedo na forçagem não ha de derrubar o programa.
//
// Sahe do namespace anonymo por ser DECLARADA em sonda.hpp: a taboa da libmpv
// consulta-a antes de carregar, para que a recusa do motor e a tela das faltas
// nunca discordem sobre o que está presente. Uma porta de fingimento, e não duas.
}  // namespace

bool nomeado_na_forcagem(std::string_view chave) {
  const char* const forcagem = std::getenv("MYSONG_SONDA_FORCA");
  if (forcagem == nullptr) return false;
  std::string_view resto(forcagem);
  while (!resto.empty()) {
    const std::size_t corte = resto.find(',');
    const std::string_view nomeada = resto.substr(
        0, corte == std::string_view::npos ? resto.size() : corte);
    if (nomeada == chave) return true;
    if (corte == std::string_view::npos) break;
    resto.remove_prefix(corte + 1);
  }
  return false;
}

// familia_com_glypho, a MESMA passagem pelas fontes installadas, com o
// predicado a perguntar pelo charset. Pelo FcFontList, e NUNCA pelo
// FcFontMatch: aquelle lista o que está installado; este CASA, e casando
// devolve fonte de substituição quando a pedida não existe. Medi-o n'esta
// machina: pedindo familia que não existe, o FcFontMatch devolveu a Noto Sans,
// cujo charset diria «sim» a glypho que Nerd Font alguma tem. Prova de glypho
// por fonte de substituição é prova falsa, e é do genero que ninguem ve.
bool familia_com_glypho(std::string_view agulha, char32_t ponto) {
  return alguma_familia(agulha, [ponto](FcPattern* fonte) {
    FcCharSet* letras = nullptr;
    return FcPatternGetCharSet(fonte, FC_CHARSET, 0, &letras) ==
               FcResultMatch &&
           letras != nullptr &&
           FcCharSetHasChar(letras, static_cast<FcChar32>(ponto)) != FcFalse;
  });
}

// familia_installada, a mesma consulta que a taboa dos requisitos faz, aberta
// a quem não é requisito: a XIROD não tranca porta alguma, e por isso não entra
// n'aquella taboa, mas o letreiro precisa de saber se ella está.
bool familia_installada(std::string_view agulha) {
  return ha_familia_de_fonte(agulha);
}

namespace {

// forcado, traduz especie e alvo na chave da taboa, e pergunta pela forçagem.
// A consulta recebe o ALVO, e a forçagem nomeia a CHAVE, que é a que o operador
// digita: a taboa faz a ponte entre as duas.
bool forcado(Especie especie, std::string_view alvo) {
  for (const Requisito& requisito : requisitos())
    if (requisito.especie == especie && requisito.alvo == alvo)
      return nomeado_na_forcagem(requisito.chave);
  return false;
}

}  // namespace

// inquerito_do_systema, arma as tres consultas de verdade, cada uma guardada
// pela forçagem. A conjuncção corta curto: forçado ausente, nem se pergunta ao
// systema, e assim a forçagem é fingimento COMPLETO, e não resposta que o
// systema depois contradiga.
Inquerito inquerito_do_systema() {
  Inquerito inquerito;
  inquerito.familia_de_fonte = [](std::string_view alvo) {
    return !forcado(Especie::FamiliaDeFonte, alvo) && ha_familia_de_fonte(alvo);
  };
  inquerito.bibliotheca = [](std::string_view alvo) {
    return !forcado(Especie::Bibliotheca, alvo) && ha_bibliotheca(alvo);
  };
  inquerito.executavel = [](std::string_view alvo) {
    return !forcado(Especie::Executavel, alvo) && ha_executavel(alvo);
  };
  return inquerito;
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//, Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
