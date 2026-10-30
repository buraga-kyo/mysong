#include "nucleo/aquisicao.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace mysong::nucleo {

std::string_view nome_da_etapa(EtapaDaBaixa etapa) {
  switch (etapa) {
    case EtapaDaBaixa::Preparando: return "Preparando pedido";
    case EtapaDaBaixa::Identificando: return "Identificando a gravação";
    case EtapaDaBaixa::Procurando: return "Procurando o áudio";
    case EtapaDaBaixa::Consultando: return "Consultando a fonte";
    case EtapaDaBaixa::Transferindo: return "Baixando o áudio";
    case EtapaDaBaixa::Convertendo: return "Convertendo o áudio";
    case EtapaDaBaixa::Capa: return "Preparando a capa";
    case EtapaDaBaixa::Letra: return "Buscando a letra";
    case EtapaDaBaixa::Canvas: return "Buscando o Canvas";
    case EtapaDaBaixa::Etiquetas: return "Gravando metadados";
    case EtapaDaBaixa::Verificando: return "Verificando o arquivo";
    case EtapaDaBaixa::Biblioteca: return "Atualizando a biblioteca";
    case EtapaDaBaixa::Pronta: return "Pronta para tocar";
  }
  return {};
}

namespace {
std::optional<double> numero(const std::string& campo) {
  if (campo.empty()) return std::nullopt;
  char* fim = nullptr;
  const double valor = std::strtod(campo.c_str(), &fim);
  if (fim != campo.c_str() + campo.size() || !std::isfinite(valor) || valor < 0)
    return std::nullopt;
  return valor;
}
}  // namespace

std::optional<ProgressoDaBaixa> le_progresso_da_baixa(std::string_view linha) {
  std::istringstream entrada{std::string(linha)};
  std::vector<std::string> campos;
  std::string campo;
  while (std::getline(entrada, campo, '|')) campos.push_back(campo);
  ProgressoDaBaixa info;
  if (campos.size() == 4 && campos[0] == "MYSONG_PP" && campos[2] == "END") {
    if (campos[3] != "started" && campos[3] != "processing") return std::nullopt;
    const auto& nome = campos[1];
    if (nome.find("Thumbnail") != std::string::npos) info.etapa = EtapaDaBaixa::Capa;
    else if (nome.find("ExtractAudio") != std::string::npos ||
             nome.find("Merger") != std::string::npos) info.etapa = EtapaDaBaixa::Convertendo;
    else return std::nullopt;
    info.detalhe = std::string(nome_da_etapa(info.etapa));
    return info;
  }
  if (campos.size() != 8 || campos[0] != "MYSONG_DL" ||
      (campos[1] != "downloading" && campos[1] != "finished")) return std::nullopt;
  info.etapa = EtapaDaBaixa::Transferindo;
  info.recebidos = numero(campos[2]);
  info.total = numero(campos[3]);
  if (!info.total || *info.total == 0) {
    info.total = numero(campos[4]);
    info.total_estimado = info.total && *info.total > 0;
  }
  if (info.total && *info.total == 0) info.total.reset();
  info.velocidade = numero(campos[5]);
  info.restante = numero(campos[6]);
  if (info.recebidos && info.total)
    info.porcentagem = static_cast<int>(std::clamp(*info.recebidos / *info.total * 100, 0.0, 100.0));
  // O último campo é um marcador fixo para detectar linhas truncadas.
  if (campos[7] != "END") return std::nullopt;
  info.detalhe = campos[1] == "finished"
      ? "Transferência encerrada; preparando processamento do áudio"
      : "Recebendo o áudio da fonte";
  return info;
}

}  // namespace mysong::nucleo
