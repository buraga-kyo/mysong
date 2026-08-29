// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS UNIDADES — src/api/unidades.hpp
// ══════════════════════════════════════════════════════════════════════════
// A traducção entre as unidades do NUCLEO e as do MPRIS. Vive em arquivo proprio e
// PURO, sem `libdbus`, e a razão está na issue: é onde se erra, e é o que se ha de
// poder provar em machina surda, sem barramento algum.
//
// As tres divergencias, e nenhuma é de gosto:
//   posição   o nucleo fala em SEGUNDOS de ponto flutuante; o MPRIS em
//             MICROSSEGUNDOS de inteiro de sessenta e quatro bits.
//   volume    o nucleo fala em PORCENTO inteiro de zero a cem; o MPRIS em
//             `double` de zero a um.
//   estado    o nucleo tem Tocando, Pausado e Parado; o MPRIS quer as cadeias
//             `Playing`, `Paused` e `Stopped`, e essas exactas.
//
// DOMÍNIO ......... valores do nucleo, e valores que chegam pelo barramento.
// CONTRA-DOMÍNIO .. os do outro lado, aparados.
// INVARIANTE ...... funcção alguma d'aqui lança, e nenhuma devolve valor fóra do
//                   arco que a especificação do MPRIS admitte.
// Q.E.D. .......... sendo tudo funcção de escalar para escalar, a bateria afere a
//                   traducção inteira sem barramento, sem som e sem terminal.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "nucleo/fila.hpp"
#include "nucleo/motor.hpp"

namespace mysong::api {

// Um MICROSSEGUNDO por milionesimo de segundo. Escripto como constante para que o
// numero appareça UMA vez: seis zeros escriptos duas vezes é um zero a menos n'uma
// d'ellas, e é assim que estes defeitos nascem.
inline constexpr std::int64_t kMicrosPorSegundo = 1000000;

// segundos_para_micros — a posição do nucleo em microssegundos. Trunca, e não
// arredonda: microssegundo a mais faria o cliente pedir uma posição que a faixa já
// não tem. Valor que não é numero, ou negativo, dá ZERO.
std::int64_t segundos_para_micros(double segundos);

// micros_para_segundos — o caminho de volta, para o `Seek` e o `SetPosition` que
// chegam pelo barramento.
double micros_para_segundos(std::int64_t micros);

// porcento_para_volume — o volume do nucleo no `double` do MPRIS. Apara-se em zero e
// um: cliente algum ha de receber volume de um vírgula dous, ainda que o nucleo o
// admittisse.
double porcento_para_volume(int porcento);

// volume_para_porcento — o caminho de volta. Apara em zero e cem, e ARREDONDA: o
// cliente que põe zero vírgula cinco espera cincoenta, e truncar daria quarenta e
// nove n'um `double` que sahisse de zero vírgula quatrocentos e noventa e nove.
int volume_para_porcento(double volume);

// estado_do_mpris — as cadeias que a especificação fixa, e essas exactas. Não são
// nomes de gosto: cliente que leia «Tocando» não sabe o que fazer com ella.
std::string_view estado_do_mpris(nucleo::Estado estado);

// repeticao_do_mpris — o nome que a especificação fixa para o `LoopStatus`, e esse
// exacto: None, Track e Playlist. Como os do estado, não são nomes de gosto; é aqui
// que a correspondencia com os tres valores d'esta Casa fica lavrada.
std::string_view repeticao_do_mpris(nucleo::Repeticao modo);

// repeticao_do_nome — o caminho de volta, para o `Set` que chega pelo barramento.
// Nome que a especificação não tem devolve VAZIO, e quem chama devolve erro nomeado:
// assentar «nenhuma» por defeito faria a Casa DESLIGAR o modo em resposta a um pedido
// que ella não entendeu, que é peior que recusar.
std::optional<nucleo::Repeticao> repeticao_do_nome(std::string_view nome);

// caminho_da_faixa — o `mpris:trackid`, que é caminho de objecto D-Bus e não cadeia
// livre: sómente letras, digitos e sublinhado nos segmentos, e ha de principiar por
// barra. O indice da fila serve de identidade, e o prefixo é o d'esta Casa.
//
// Fila vazia dá `/org/mpris/MediaPlayer2/TrackList/NoTrack`, que é o que a
// especificação manda para «não ha faixa». Inventar um caminho alli faria o cliente
// crer que ha faixa e pedir-lhe metadados que não existem.
std::string caminho_da_faixa(std::size_t indice, bool ha_faixa);

// url_do_arquivo — o `xesam:url`: `file://` mais o caminho com percent-encoding. Não
// se escapa a barra, que ella é a estructura do caminho; escapa-se tudo o mais que não
// seja do arco livre do RFC 3986.
std::string url_do_arquivo(std::string_view caminho);

}  // namespace mysong::api

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
