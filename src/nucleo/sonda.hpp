// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA SONDA — src/nucleo/sonda.hpp
// ══════════════════════════════════════════════════════════════════════════
// Colhe o estado dos REQUISITOS de que o tocador depende, e nada mais faz:
// não pinta tela, não escreve em sahida alguma, não sahe do programa. Quem
// julga o que fazer com a falta é a tela; quem sabe o que falta é esta sonda.
//
// A FONTE DA VERDADE ENTRA POR PARÂMETRO, e isto é a substancia do modulo e
// não commodidade: numa máquina em que os quatro requisitos estão presentes, a
// sonda que só soubesse consultar o systema real nunca correria o caminho da
// FALTA, que é justamente o caminho que importa. Injectado o inquerito, os dous
// caminhos provam-se com dublê, sem se mexer no systema.
//
// DOMÍNIO ......... um INQUERITO: tres consultas que respondem se ha familia de
//                   fonte, se ha bibliotheca, se ha executavel. Nunca o systema
//                   directamente.
// CONTRA-DOMÍNIO .. um RELATORIO: a taboa dos requisitos, cada um com o seu
//                   estado, na ordem de declaração e sem buraco.
// INVARIANTE ...... sondar() é pura quanto ao mundo: não lê variavel de
//                   ambiente, não abre arquivo, não emitte byte, não lança
//                   excepção pela borda. Consulta que falhe conta-se FALTA, e
//                   jamais se confunde com requisito presente.
// Q.E.D. .......... havendo o inquerito por parâmetro, o caminho do impedimento
//                   é observavel onde nada falta; logo a recusa de abrir deixa
//                   de ser promessa e passa a ser asserção provada.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <functional>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

// A CLASSE de fonte que esta obra exige, pela agulha que o nome d'ella ha de
// trazer. Vive aqui, e não em duas cadeias soltas: a taboa dos requisitos
// pergunta se ha familia assim, e a capa pergunta se ELLA desenha o sextante.
// Fossem duas, o dia em que a Casa trocasse de fonte deixaria uma para traz, e
// a sonda diria «presente» sobre uma familia e a capa sobre outra.
inline constexpr std::string_view CLASSE_DA_FONTE = "nerd";

// A GRAVIDADE da falta, que é o que decide o destino do programa. Duas, e
// sómente duas: o Impedimento tranca a porta, o Aviso apenas a rannge.
enum class Gravidade { Impedimento, Aviso };

// A ESPECIE de consulta que apura o requisito: ha tres maneiras de perguntar
// ao systema, e cada requisito diz qual d'ellas lhe cabe. Sem isto, sondar()
// teria de conhecer cada requisito pelo nome, e acrescentar requisito novo
// obrigaria a mexer na laçada em vez de mexer sómente na taboa.
enum class Especie { FamiliaDeFonte, Bibliotheca, Executavel };

// Um REQUISITO da obra. A `chave` é o nome curto, que serve de identidade á
// prova e á forçagem; o `nome` é como se annuncia ao olho humano; o `alvo` é o
// que se procura de facto, e vai crú á consulta; e o `remedio` é UMA linha que
// diz o que se faz, e não um tractado de installação.
struct Requisito {
  std::string_view chave;
  std::string_view nome;
  Gravidade gravidade;
  Especie especie;
  std::string_view alvo;
  std::string_view remedio;
};

// O VEREDICTO sobre um requisito. Guarda-se o requisito INTEIRO junto do
// veredicto, de sorte que quem receba o relatorio não precise consultar taboa
// alguma para saber o nome, a gravidade e o remedio d'aquillo que falta.
struct Veredicto {
  Requisito requisito;
  bool presente;
};

// O RELATORIO: a taboa INTEIRA, e não sómente o que falta. Quem quer a falta
// pede faltas(); quem quer o diagnostico completo, que é o caso do modo
// --sonda, tem os estados todos, presentes inclusos.
struct Relatorio {
  std::vector<Veredicto> estados;

  bool ha_falta() const;
  bool ha_impedimento() const;
  std::vector<Veredicto> faltas() const;  // impedimentos primeiro, ordem estavel
};

// O INQUERITO: as tres consultas que a sonda faz ao mundo, embrulhadas de sorte
// que a prova as substitua por dublê em duas linhas, sem herança e sem macro.
// Consulta VAZIA responde ausente, e nunca presente: inquerito mal montado ha
// de accusar falta, jamais dar por bom aquillo que não sabe.
struct Inquerito {
  std::function<bool(std::string_view)> familia_de_fonte;
  std::function<bool(std::string_view)> bibliotheca;
  std::function<bool(std::string_view)> executavel;
};

// A taboa dos requisitos d'esta obra, na ordem em que se declaram.
const std::vector<Requisito>& requisitos();

// Sonda os requisitos pelo inquerito dado. Pura quanto ao mundo: nem ambiente,
// nem tela, nem sahida do programa. Nunca lança pela borda.
Relatorio sondar(const Inquerito& inquerito);

// Diz se a forçagem nomeia esta CHAVE da taboa. Declarada aqui por a taboa da
// libmpv a consultar: fingida a ausencia, nem a sonda nem o motor a contradizem.
bool nomeado_na_forcagem(std::string_view chave);

// O inquerito que consulta o systema de verdade, e o UNICO logar d'esta obra a
// ler variavel de ambiente: MYSONG_SONDA_FORCA nomeia, por chave curta e
// separadas por virgula, os requisitos que se hão de ter por ausentes.
Inquerito inquerito_do_systema();

// familia_com_glypho — diz se ALGUMA familia installada cujo nome traga a
// agulha desenha o ponto de codigo pedido. Pergunta-se pela CLASSE de fonte
// que a obra exige, e nunca por um nome chumbado: quem corre outra Nerd Font
// que não a d'esta machina ha de obter a mesma resposta.
//
// Serve á capa, que precisa de saber se o sextante (U+1FB00) tem glypho antes
// de o pedir ao chafa: sem elle, o sextante sahe quadrículo vazio.
bool familia_com_glypho(std::string_view agulha, char32_t ponto);

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
