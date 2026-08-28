// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DOS AJUSTES — src/nucleo/ajustes.cpp
// ══════════════════════════════════════════════════════════════════════════
// A obra dos ajustes, em partes que se não misturam: o LEITOR, que parte o
// texto em pares e nada sabe do mundo; o RESOLVEDOR, que escolhe entre os
// quatro degraus e nada abre; e as funcções que tocam disco e ambiente.
//
// INVARIANTE ...... arquivo algum se abre para escripta n'esta unidade.
// Q.E.D. .......... puros o leitor e o resolvedor, a bateria prova o formato e
//                   a precedencia inteira sem tocar disco nem ambiente.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/ajustes.hpp"

#include <charconv>
#include <cstdlib>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

namespace mysong::nucleo {

// nome_da_origem — a palavra do --sonda. Switch exhaustivo, e não taboa: origem
// nova accende aviso do compilador em vez de sahir calada como «sem nome».
std::string_view nome_da_origem(Origem origem) {
  switch (origem) {
    case Origem::Argumento: return "argumento";
    case Origem::Ambiente: return "ambiente";
    case Origem::Arquivo: return "arquivo";
    case Origem::Padrao: return "padrão";
  }
  return "origem sem nome";
}

// queixa — accrescenta, até o tecto. Alcançado elle, deixa-se UMA linha a dizer
// que ha mais: é o que faz um binario passado como conf caber n'uma tela.
void Ajustes::queixa(std::string dito) {
  if (queixas.size() < QUEIXAS_NO_MAXIMO) {
    queixas.push_back(std::move(dito));
  } else if (queixas.size() == QUEIXAS_NO_MAXIMO) {
    queixas.push_back("e ha mais queixas, que o tecto de " +
                      std::to_string(QUEIXAS_NO_MAXIMO) + " se alcançou");
  }
}

// aparar — tira os brancos das DUAS pontas, e sómente das pontas: branco no
// MEIO do valor é do valor, que caminho com espaço é caminho legitimo.
std::string_view aparar(std::string_view texto) {
  const auto branco = [](char letra) {
    return letra == ' ' || letra == '\t' || letra == '\r' || letra == '\v' ||
           letra == '\f';
  };
  while (!texto.empty() && branco(texto.front())) texto.remove_prefix(1);
  while (!texto.empty() && branco(texto.back())) texto.remove_suffix(1);
  return texto;
}

// corta_commentario — o `#` abre commentario até o fim da linha, em QUALQUER
// ponto, e não ha aspas nem escape que o façam literal. O limite é DECLARADO, e
// não descuido: caminho que traga cerquilha fica inexprimivel, e regra com
// excepção seria regra que o operador não adivinha olhando o proprio arquivo.
std::string_view corta_commentario(std::string_view linha) {
  const std::size_t cerquilha = linha.find('#');
  if (cerquilha == std::string_view::npos) return linha;
  return linha.substr(0, cerquilha);
}

// ler_pares — o LEITOR do formato: linhas «chave = valor», com o `#` a abrir
// commentario, os brancos aparados nas pontas, e a linha vazia ignorada. A
// chave repetida NÃO se resolve aqui: os pares sahem na ordem em que vieram, e
// quem os consome adeante toma o ultimo, que é o costume de todo arquivo de
// linhas e o que deixa o operador corrigir accrescentando no fim.
//
// Nada aqui é fatal. Linha que se não entende vira QUEIXA e segue: arquivo
// velho, escripto para uma versão anterior d'esta obra, não ha de impedir a
// obra de abrir, que é o que a issue #67 manda por extenso.
std::vector<Par> ler_pares(std::string_view texto, Ajustes* ajustes) {
  std::vector<Par> pares;
  std::size_t numero = 0;
  while (!texto.empty()) {
    const std::size_t quebra = texto.find('\n');
    const bool ultima = quebra == std::string_view::npos;
    const std::string_view crua = texto.substr(0, ultima ? texto.size() : quebra);
    texto.remove_prefix(crua.size() + (ultima ? 0 : 1));
    const std::string onde = "linha " + std::to_string(++numero) + ": ";
    if (crua.size() > LINHA_NO_MAXIMO) {
      ajustes->queixa(onde + "comprida de mais; ignorada");
      continue;
    }
    const std::string_view linha = aparar(corta_commentario(crua));
    if (linha.empty()) continue;
    const std::size_t egual = linha.find('=');
    if (egual == std::string_view::npos) {
      ajustes->queixa(onde + "sem o signal de egual; ignorada");
      continue;
    }
    const std::string_view chave = aparar(linha.substr(0, egual));
    if (chave.empty()) {
      ajustes->queixa(onde + "sem chave antes do egual; ignorada");
      continue;
    }
    for (const Par& antigo : pares)
      if (antigo.chave == chave)
        ajustes->queixa(onde + "a chave «" + std::string(chave) +
                        "» já veio na linha " + std::to_string(antigo.linha) +
                        "; vale a ultima");
    pares.push_back({numero, std::string(chave),
                     std::string(aparar(linha.substr(egual + 1)))});
  }
  return pares;
}

namespace {

// egual_sem_caixa — compara cego á caixa, em ASCII, e faz a caixa á mão pela
// razão que o contem_insensivel da sonda já tem escripta: sob locale turco o
// 'I' desce a caractere que não é 'i', e «YouTube» fugiria da comparação por
// motivo que ninguem havia de suspeitar.
bool egual_sem_caixa(std::string_view esta, std::string_view aquella) {
  const auto baixa = [](char letra) {
    return letra >= 'A' && letra <= 'Z' ? static_cast<char>(letra | 0x20)
                                        : letra;
  };
  if (esta.size() != aquella.size()) return false;
  for (std::size_t passo = 0; passo < esta.size(); ++passo)
    if (baixa(esta[passo]) != baixa(aquella[passo])) return false;
  return true;
}

// inteiro_de — o numero INTEIRAMENTE consumido, e nunca o prefixo d'elle: «70
// lixo» não é setenta. Vae por from_chars, e não por stoi: aquelle accusa o
// estouro em vez de o dobrar, e não lança; excepção pela borda d'este modulo
// derrubaria a obra por causa de um erro de dedo no arquivo do operador.
std::optional<int> inteiro_de(std::string_view texto) {
  if (!texto.empty() && texto.front() == '+') texto.remove_prefix(1);
  int valor = 0;
  const char* const fim = texto.data() + texto.size();
  const std::from_chars_result colhido =
      std::from_chars(texto.data(), fim, valor);
  if (colhido.ec != std::errc() || colhido.ptr != fim) return std::nullopt;
  return valor;
}

}  // namespace

// fonte_de — as tres da issue #56, e sómente ellas. Nome que não é nenhuma
// d'ellas devolve vazio, e não a primeira da lista: acceitar por approximação
// faria o operador buscar no Spotify a pensar que buscava no YouTube.
std::optional<Fonte> fonte_de(std::string_view texto) {
  if (egual_sem_caixa(texto, "youtube")) return Fonte::YouTube;
  if (egual_sem_caixa(texto, "youtube-music")) return Fonte::YouTubeMusic;
  if (egual_sem_caixa(texto, "spotify")) return Fonte::Spotify;
  return std::nullopt;
}

// chave_da_fonte — o inverso do fonte_de, e as duas listas hão de bater. Ficam
// visinhas de proposito: fonte nova acrescentada n'uma e esquecida na outra
// accende aviso do compilador nos dous switches, e não em nenhum.
std::string_view chave_da_fonte(Fonte fonte) {
  switch (fonte) {
    case Fonte::YouTube: return "youtube";
    case Fonte::YouTubeMusic: return "youtube-music";
    case Fonte::Spotify: return "spotify";
  }
  return "fonte sem nome";
}

std::optional<int> volume_de(std::string_view texto) {
  const std::optional<int> numero = inteiro_de(texto);
  if (!numero || *numero < 0 || *numero > VOLUME_DA_CASA) return std::nullopt;
  return numero;
}

// baixas_de — de uma até o tecto, e zero recusa-se: obreiro nenhum é fila que
// nunca anda, e o operador ficaria a ver a baixa «na espera» para sempre sem
// entender por que. O tecto está no cabeçalho, com a razão d'elle.
std::optional<std::size_t> baixas_de(std::string_view texto) {
  const std::optional<int> numero = inteiro_de(texto);
  if (!numero || *numero < 1 || *numero > BAIXAS_NO_MAXIMO) return std::nullopt;
  return static_cast<std::size_t>(*numero);
}

// resolver — a ESCADA, assentada de baixo para cima: o padrão primeiro, e o
// arquivo a escrever por cima. Percorrer os pares na ORDEM em que vieram é o
// que faz a chave repetida valer a ultima, sem regra propria para isso: a
// segunda occorrencia sobrescreve a primeira, e é tudo.
void resolver(const Degraus& degraus,
              const std::filesystem::path& padrao_do_acervo,
              const Aferidor& ha_directorio, Ajustes* ajustes) {
  ajustes->acervo = {padrao_do_acervo, Origem::Padrao};
  for (const Par& par : degraus.arquivo) {
    const std::string onde = "linha " + std::to_string(par.linha) + ": ";
    if (par.chave == "acervo") {
      if (ha_directorio(par.valor))
        ajustes->acervo = {std::filesystem::path(par.valor), Origem::Arquivo};
      else
        ajustes->queixa(onde + "acervo «" + par.valor +
                       "» não é directorio que exista; vale o de baixo");
    } else if (par.chave == "volume") {
      if (const auto numero = volume_de(par.valor))
        ajustes->volume = {*numero, Origem::Arquivo};
      else
        ajustes->queixa(onde + "volume «" + par.valor +
                       "» não é numero de zero a cem; vale o degrau de baixo");
    } else if (par.chave == "fonte_da_busca") {
      if (const auto fonte = fonte_de(par.valor))
        ajustes->fonte_da_busca = {*fonte, Origem::Arquivo};
      else
        ajustes->queixa(onde + "fonte_da_busca «" + par.valor +
                       "» não é youtube, youtube-music nem spotify");
    } else if (par.chave == "baixas_simultaneas") {
      if (const auto quantas = baixas_de(par.valor))
        ajustes->baixas_simultaneas = {*quantas, Origem::Arquivo};
      else
        ajustes->queixa(onde + "baixas_simultaneas «" + par.valor +
                       "» não é numero de um a oito");
    } else {
      ajustes->queixa(onde + "chave desconhecida «" + par.chave + "»; ignorada");
    }
  }
  // O AMBIENTE, e vae CRÚ: quem poz a variavel no perfil do shell manda, e o
  // programa não julga o caminho d'ella. Aferil-a mudaria o acervo debaixo dos
  // pés de quem aponta para monte de rede que ainda não montou, e a variavel
  // existe justamente para esse caso.
  if (degraus.acervo_do_ambiente)
    ajustes->acervo = {std::filesystem::path(*degraus.acervo_do_ambiente),
                      Origem::Ambiente};
  // O ARGUMENTO, que é o degrau de cima, e este AFERE-SE: quem o digita está a
  // olhar para a tela agora, e ha de saber já que errou o caminho.
  if (degraus.acervo_do_argumento) {
    if (ha_directorio(*degraus.acervo_do_argumento))
      ajustes->acervo = {std::filesystem::path(*degraus.acervo_do_argumento),
                        Origem::Argumento};
    else
      ajustes->queixa("--acervo «" + *degraus.acervo_do_argumento +
                     "» não é directorio que exista; vale o de baixo");
  }
}

// caminho_da_configuracao — o mesmo desenho do caminho_do_indice, com UMA
// differença dita de proposito: aquelle CREA o directorio, porque o índice é
// nosso e nasce d'esta obra; este não crea cousa alguma, porque o arquivo é do
// operador. Directorio creado por nós, vazio, faria o operador crer que a obra
// escreveu alli o que elle havia de escrever á mão.
std::filesystem::path caminho_da_configuracao() {
  const char* const posto = std::getenv("XDG_CONFIG_HOME");
  std::filesystem::path raiz;
  if (posto != nullptr && posto[0] != '\0') {
    raiz = std::filesystem::path(posto);
  } else {
    const char* const casa = std::getenv("HOME");
    if (casa == nullptr) return {};
    raiz = std::filesystem::path(casa) / ".config";
  }
  return raiz / "mysong" / "mysong.conf";
}

// ler_o_arquivo — abre para LEITURA, e não ha n'esta unidade uma segunda porta
// que escreva. O tecto de tamanho existe porque o caminho pode apontar para
// cousa que não é configuração: sem elle, um binario de meio giga viraria meio
// giga de cadeia na pilha antes de a primeira linha se ler.
EstadoDoArquivo ler_o_arquivo(const std::filesystem::path& caminho,
                              std::string* texto, Ajustes* ajustes) {
  std::error_code erro;
  if (caminho.empty() || !std::filesystem::exists(caminho, erro))
    return EstadoDoArquivo::Ausente;
  if (!std::filesystem::is_regular_file(caminho, erro)) {
    ajustes->queixa("«" + caminho.string() +
                    "» não é arquivo regular; valem os padrões");
    return EstadoDoArquivo::Illegivel;
  }
  std::ifstream porta(caminho, std::ios::binary);
  if (!porta) {
    ajustes->queixa("não se pôde ler «" + caminho.string() +
                    "»; valem os padrões");
    return EstadoDoArquivo::Illegivel;
  }
  texto->resize(ARQUIVO_NO_MAXIMO);
  porta.read(texto->data(), static_cast<std::streamsize>(ARQUIVO_NO_MAXIMO));
  const std::size_t colhido = static_cast<std::size_t>(porta.gcount());
  texto->resize(colhido);
  if (colhido == ARQUIVO_NO_MAXIMO)
    ajustes->queixa("o arquivo passa de um megabyte: leu-se só o começo");
  return EstadoDoArquivo::Lido;
}

// padrao_do_acervo — o chão da escada, e o mesmo de sempre: `~/Música`. Sem
// HOME, devolve vazio, e ahi a varredura não acha nada, que é o que já succedia.
std::filesystem::path padrao_do_acervo() {
  const char* const casa = std::getenv("HOME");
  if (casa == nullptr) return {};
  return std::filesystem::path(casa) / "Música";
}

// ajustes_do_systema — a montagem. Tudo o que toca o mundo está n'estas vinte
// linhas, e tudo o mais d'este arquivo é puro: é o que faz a bateria alcançar
// o formato inteiro e a precedencia inteira sem tocar disco nem ambiente.
Ajustes ajustes_do_systema(const std::optional<std::string>& do_argumento) {
  Ajustes ajustes;
  ajustes.arquivo = caminho_da_configuracao();
  Degraus degraus;
  degraus.acervo_do_argumento = do_argumento;
  const char* const posto = std::getenv("MYSONG_ACERVO");
  if (posto != nullptr && posto[0] != '\0') degraus.acervo_do_ambiente = posto;
  std::string texto;
  ajustes.estado = ler_o_arquivo(ajustes.arquivo, &texto, &ajustes);
  if (ajustes.estado == EstadoDoArquivo::Lido)
    degraus.arquivo = ler_pares(texto, &ajustes);
  // O aferidor de verdade. O erro do systema colhe-se no error_code e não em
  // excepção: caminho em monte que se desligou responde «não é directorio», e
  // não derruba o programa por lançar de dentro do resolvedor.
  const Aferidor ha_directorio = [](const std::filesystem::path& caminho) {
    std::error_code erro;
    return std::filesystem::is_directory(caminho, erro);
  };
  resolver(degraus, padrao_do_acervo(), ha_directorio, &ajustes);
  return ajustes;
}

namespace {

// nome_do_estado — a palavra do arquivo no diagnostico. O caso «ausente» diz
// tambem o que succedeu por causa d'elle, que é a duvida seguinte de quem lê.
std::string_view nome_do_estado(EstadoDoArquivo estado) {
  switch (estado) {
    case EstadoDoArquivo::Ausente: return "ausente, e valem os padrões";
    case EstadoDoArquivo::Lido: return "lido";
    case EstadoDoArquivo::Illegivel: return "não se leu";
  }
  return "estado sem nome";
}

// linha_do_ajuste — a chave guarnecida á largura da maior, o valor, e a origem
// entre parenthesis. O valor NÃO se trunca: caminho cortado n'um diagnostico é
// o defeito, e não o remedio.
void linha_do_ajuste(std::string* texto, std::string_view chave,
                     const std::string& valor, Origem origem) {
  *texto += "  ";
  texto->append(chave);
  texto->append(chave.size() < 20 ? 20 - chave.size() : 1, ' ');
  *texto += valor;
  *texto += "  (";
  texto->append(nome_da_origem(origem));
  *texto += ")\n";
}

}  // namespace

std::string texto_dos_ajustes(const Ajustes& ajustes) {
  std::string texto = "\nmysong: os ajustes em vigor, e de onde vieram\n\n";
  linha_do_ajuste(&texto, "acervo", ajustes.acervo.valor.string(),
                  ajustes.acervo.origem);
  linha_do_ajuste(&texto, "volume", std::to_string(ajustes.volume.valor),
                  ajustes.volume.origem);
  linha_do_ajuste(&texto, "fonte_da_busca",
                  std::string(chave_da_fonte(ajustes.fonte_da_busca.valor)),
                  ajustes.fonte_da_busca.origem);
  linha_do_ajuste(&texto, "baixas_simultaneas",
                  std::to_string(ajustes.baixas_simultaneas.valor),
                  ajustes.baixas_simultaneas.origem);
  // O CAMINHO vae sempre, ainda que o arquivo não exista: sem elle, quem
  // escreveu o arquivo no logar errado não tem como descobrir qual é o certo.
  texto += "\n  arquivo: " + ajustes.arquivo.string() + " (";
  texto.append(nome_do_estado(ajustes.estado));
  texto += ")\n";
  // As QUEIXAS, e a linha que as apresenta diz logo que ellas não trancam a
  // porta: quem vê lista de erros n'um diagnostico suppõe que o programa parou.
  if (!ajustes.queixas.empty()) {
    texto += "\n  queixas, que não impedem a obra de abrir:\n";
    for (const std::string& queixa : ajustes.queixas)
      texto += "    " + queixa + "\n";
  }
  return texto;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
