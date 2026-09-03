// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA ONDA — src/nucleo/onda.hpp
// ══════════════════════════════════════════════════════════════════════════
// A ENVOLVENTE de amplitude da faixa INTEIRA, ao modo da onda do SoundCloud:
// mil e vinte e quatro pontos em [0,1], um por balde de tempo, a RMS de cada
// balde normalizada ao máximo da faixa. Colhe-se UMA vez, pelo ffmpeg, e o que
// se colheu guarda-se em cache: a leitura seguinte não corre programa algum.
//
// DOMÍNIO ......... o caminho de uma faixa do acervo.
// CONTRA-DOMÍNIO .. mil e vinte e quatro pontos em [0,1]; ou a onda VAZIA com
//                   a razão dita, e ahi quem chama pinta a barra chata.
// INVARIANTE ...... colher é CARO, que o ffmpeg decodifica a faixa inteira;
//                   d'onde se guarda por caminho, tamanho e mtime, como a capa.
// Q.E.D. .......... sendo puras a linha de commando, a conta da envolvente, a
//                   chave e o formato do cache, a bateria afere-as sem ffmpeg.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

// Quantos pontos a onda tem. Mil e vinte e quatro, e não a largura da tela: a
// onda guarda-se UMA vez e a tela dobra-a á largura que tiver, d'onde
// redimensionar a janella não manda correr o ffmpeg outra vez.
inline constexpr std::size_t PONTOS_DA_ONDA = 1024;

// A ONDA. Vazia quer dizer «não ha», e nunca «silencio»: faixa calada dá mil e
// vinte e quatro zeros, que é cousa differente de faixa que o ffmpeg não leu.
struct Onda {
  std::vector<float> pontos;
  bool pronta() const noexcept { return !pontos.empty(); }
};

// linha_de_commando_da_onda — o argv EXACTO. PURA, e por isso a bateria afere
// o que se HA DE correr sem correr cousa alguma, como esta Casa faz com o
// chafa. Oito mil amostras por segundo e um canal só: quer-se a FÓRMA da
// faixa, e não a fidelidade d'ella.
std::vector<std::string> linha_de_commando_da_onda(
    const std::filesystem::path& faixa);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
