#pragma once

// -------- Constantes globais com informações de compilação --------
extern const String buildDateTime = String(__DATE__) + " " + String(__TIME__);  // Data e hora da compilação
extern const String buildVersion = String(__DATE__) + " " + String(__TIME__);   // Versão
extern const String buildFile = String(__FILE__);                               // Nome do arquivo atual (ex: "main.cpp")
