# Manual de Criação de Controles (remotes.json)

O arquivo de controle é um documento de texto no formato JSON. Ele funciona como um "mapa" que diz ao dispositivo qual cor o botão deve ter, qual o nome dele e qual sinal infravermelho (IR) ele deve disparar.

---

## 1. Estrutura Básica do Arquivo

O arquivo é composto por **Modelos** (ex: "TV Sala", "Ar Condicionado") e, dentro de cada modelo, uma lista de **Botões**.

```json
{
  "NOME_DO_MODELO": {
    "buttons": [
      {
        "name": "NOME_DO_BOTAO",
        "type": "button",
        "protocol": "PROTOCOLO",
        "code": "CODIGO_HEX",
        "bits": 32,
        "span": 1,
        "background": "HEX_COR"
      }
    ]
  }
}
```

---

## 2. Propriedades dos Botões

Para cada botão, você pode configurar as seguintes propriedades:

| Propriedade  | Descrição                                                              | Exemplo            |
|--------------|------------------------------------------------------------------------|--------------------|
| `name`       | Texto que aparecerá dentro do botão.                                   | `"POWER"`, `"VOL +"` |
| `type`       | `"button"` para comandos, `"space"` para criar um vazio.               | `"button"`         |
| `protocol`   | Protocolo do sinal IR.                                                 | `"NEC"`            |
| `code`       | Código do comando (hex com ou sem `0x`, ou decimal).                   | `"0x20DF10EF"`     |
| `bits`       | Tamanho do sinal em bits (geralmente 32 para NEC).                     | `32`               |
| `span`       | Quantas colunas o botão ocupa (1, 2 ou 3). Padrão: `1`.               | `2`                |
| `rowSpan`    | Quantas linhas o botão ocupa. Padrão: `1`.                             | `2`                |
| `background` | Cor de fundo do botão em hexadecimal, sem `#`.                         | `"cf5656"`         |
| `color`      | Cor do texto do botão em hexadecimal, sem `#` (opcional).              | `"ffffff"`         |
| `fontSize`   | Tamanho da fonte do texto (opcional).                                  | `"10px"`           |

**Protocolos suportados:** `NEC`, `SONY`, `RC5`, `RC6`, `SAMSUNG`, `NIKAI`, `LG`, `JVC`, `WHYNTER`

---

## 3. Sistema de Layout (Grid)

O grid do IRHub é dividido em **3 colunas**. Use `span` para controlar a largura dos botões:

| `span` | Largura ocupada         | Uso típico                  |
|--------|-------------------------|-----------------------------|
| `1`    | 1/3 da linha (padrão)   | Botões numéricos            |
| `2`    | 2/3 da linha            | Vol+/Vol-, Canal+/Canal-    |
| `3`    | Largura total da linha  | Botão de Power no topo      |

Use `rowSpan: 2` para criar botões que ocupam duas linhas de altura (ideal para botões verticais laterais).

---

## 4. Criando Espaçamentos

Para deixar um espaço vazio entre botões, use o tipo `space`:

```json
{ "type": "space", "span": 1 }
```

---

## 5. Exemplo Completo

```json
{
  "Minha TV": {
    "buttons": [
      {
        "name": "LIGAR",
        "type": "button",
        "span": 3,
        "background": "e31b1b",
        "protocol": "NEC",
        "code": "0x20DF10EF",
        "bits": 32
      },
      {
        "name": "VOL +",
        "type": "button",
        "span": 1,
        "background": "333",
        "protocol": "NEC",
        "code": "0x20DF40BF",
        "bits": 32
      },
      { "type": "space", "span": 1 },
      {
        "name": "CH +",
        "type": "button",
        "span": 1,
        "background": "333",
        "protocol": "NEC",
        "code": "0x20DF00FF",
        "bits": 32
      }
    ]
  }
}
```

---

## 6. Como instalar o controle no dispositivo

1. Crie o arquivo no seu computador e salve como `remotes.json`.
2. Acesse a página inicial (Home) do seu **IRHub-8266**.
3. Vá até a seção **"Controles Remotos"**.
4. Clique em **"Escolher arquivo"** e selecione o seu `remotes.json`.
5. Clique em **"📤 Importar"**.
6. O novo modelo aparecerá instantaneamente no menu de seleção.

> **Dica:** Use a aba **IR** do sistema para capturar os códigos do seu controle físico original. Aponte o controle para o IRHub, pressione a tecla e copie o `Protocol`, `Code` e `Bits` que aparecerem no histórico.
