// O ESPIÃO da capa (issue #83): artista, titulo, segundos e um MP3 de
// rascunho; corre a caça pelos MESMOS degraus da corrida (casamento, CAA,
// embute) e relê o quadro ao fim. Custa até tres requisições, uma por
// segundo, e NUNCA se aponta ao acervo de verdade: faixa de rascunho.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "nucleo/caa.hpp"
#include "nucleo/capa.hpp"

namespace nu = mysong::nucleo;

int main(int argc, char** argv) {
  if (argc < 5) {
    std::fprintf(stderr,
                 "uso: espia_capa <artista> <titulo> <seg> <faixa.mp3>\n");
    return 2;
  }
  nu::CacaDeCapa porque = nu::CacaDeCapa::Duvidosa;
  const std::string release = nu::casa_release(
      argv[1], argv[2], std::atoi(argv[3]), nu::consulta_mb_com_estado,
      &porque);
  if (release.empty()) {
    std::printf("não casou: %s\n",
                std::string(nu::palavra_da_caca(porque)).c_str());
    return 1;
  }
  std::printf("release: %s\ncapa:    %s\n", release.c_str(),
              nu::url_da_capa(release).c_str());
  long estado = 0;
  std::string arte;
  // Em dous passos, pela mesma razão medida na lavra da caça: a ordem de
  // avaliação de argumentos é livre, e o estado ha de ser lido DEPOIS.
  const nu::DesfechoMB resposta =
      nu::consulta_mb_com_estado(nu::url_da_capa(release), &arte, &estado);
  const nu::DesfechoDaCapa dito = nu::desfecho_da_capa(resposta, estado);
  std::printf("estado:  %ld (%zu octetos)\n", estado, arte.size());
  if (dito != nu::DesfechoDaCapa::Achada) return 1;
  if (!nu::embute_arte(argv[4], arte)) {
    std::puts("a etiqueta não se escreveu");
    return 1;
  }
  const std::string relida = nu::arte_embutida(argv[4]);
  std::printf("relida:  %zu octetos%s\n", relida.size(),
              relida == arte ? ", identicos aos baixados" : ", DIFFEREM");
  return relida == arte ? 0 : 1;
}

//   Da lavra do eminente Doutor BRAGA US. — buraga-kyo ✒
