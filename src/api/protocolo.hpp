// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROTOCOLO — src/api/protocolo.hpp
// ══════════════════════════════════════════════════════════════════════════
// O CEREBRO do socket, e sómente o cerebro: uma linha entra, uma linha sahe.
// Socket algum se nomeia aqui, nem descriptor, nem cliente; d'isso cuida o
// tractado do socket, que é o braço. É a mesma repartição que a Casa fez entre
// o Motor, que é potencia, e o Tocador, que é ordem.
//
// DOMÍNIO ......... o Tocador do nucleo, emprestado por referencia; e uma linha
//                   de texto que chegou de fóra e que pode ser qualquer cousa.
// CONTRA-DOMÍNIO .. uma linha de JSON, SEM o \n do enquadramento, que quem
//                   transporta acrescenta. Ou cadeia VAZIA, que significa «nada
//                   a responder» e é o que a linha em branco merece.
// INVARIANTE ...... verbo algum fica MUDO. Todo caminho d'esta obra devolve JSON
//                   ou a cadeia vazia da linha em branco: o que não se conhece,
//                   o que esta instancia não ergueu, e o que o nucleo
//                   recusou têm cada um o seu codigo, e os tres se distinguem.
//                   Erro que volta como silêncio é o que a issue proibiu com
//                   essas palavras.
// Q.E.D. .......... sendo funcção pura de cadeia para cadeia sobre um Tocador
//                   que já se prova por dublê, os treze verbos provam-se em
//                   machina surda, sem abrir socket algum; e é d'ahi que sobram
//                   cinco casos, e não trinta, para a bateria que precisa de
//                   estado global do systema.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <string>
#include <string_view>

#include "nucleo/tocador.hpp"

// A BIBLIOTHECA e o ESTALEIRO vêm declarados ADIANTE: quem inclue este
// cabeçalho não herda o sqlite3 nem os fios da fila de baixa.
namespace mysong::nucleo {
class Biblioteca;
class Estaleiro;
}  // namespace mysong::nucleo

namespace mysong::api {

// A VERSÃO do contracto. Sobe quando a mudança pode quebrar cliente que já
// exista; o verbo «versao» a devolve, para que o outro lado a possa exigir.
constexpr int kVersaoDoProtocolo = 1;

// OS ARREDORES: as peças do nucleo que os verbos da issue #65 pedem e que o
// Tocador não conhece. Emprestadas, e ANNULAVEIS, que quem ergue o servidor
// pode não ter índice nem fila de baixa; o protocolo diz a falta em vez de a
// calar. Quem as empresta ha de mantê-las vivas mais tempo que o servidor.
struct Arredores {
  const nucleo::Biblioteca* livraria = nullptr;
  nucleo::Estaleiro* estaleiro = nullptr;
};

// Uma linha entra, uma linha sahe. Cadeia vazia é «nada a responder».
std::string responde(nucleo::Tocador& tocador, std::string_view linha);

// A mesma, com os arredores. A de cima delega n'esta com arredores vazios.
std::string responde(nucleo::Tocador& tocador, const Arredores& arredores,
                     std::string_view linha);

}  // namespace mysong::api

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
