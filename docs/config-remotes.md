Este guia foi elaborado para que você possa criar seus próprios controles remotos personalizados para o **IRHub-8266**. O sistema utiliza um arquivo chamado `remotes.json` que define a aparência e os comandos de cada botão.

---

# Manual de Criação de Controles (remotes.json)

O arquivo de controle é um documento de texto no formato JSON. Ele funciona como um "mapa" que diz ao dispositivo qual cor o botão deve ter, qual o nome dele e qual sinal infravermelho (IR) ele deve disparar.

### 1. Estrutura Básica do Arquivo

O arquivo é composto por **Modelos** (ex: "TV Sala", "Ar Condicionado") e, dentro de cada modelo, uma lista de **Botões**.

JSON

```
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

### 2. Propriedades dos Botões (Dicionário)

Para cada botão, você pode configurar as seguintes propriedades:

|Propriedade|Descrição|Exemplo|
|---|---|---|
|**name**|O texto que aparecerá dentro do botão.|`"POWER"`, `"VOL +"`|
|**type**|Use `"button"` para comandos ou `"space"` para criar um vazio.|`"button"`|
|**protocol**|O protocolo do sinal (NEC, SONY, LG, SAMSUNG, etc).|`"NEC"`|
|**code**|O código hexadecimal do comando (obtido na aba "IR" do sistema).|`"0x20DF10EF"`|
|**bits**|Tamanho do sinal em bits (geralmente 32 para NEC).|`32`|
|**background**|Cor de fundo do botão (em hexadecimal, sem o #).|`"cf5656"` (Vermelho)|
|**color**|Cor do texto do botão (opcional).|`"fff"` (Branco)|

Exportar para as Planilhas

---

### 3. Opções de Layout (Dimensionamento)

Para deixar o controle com uma aparência profissional, você pode usar o sistema de **Spans** (espaçamento). O grid do IRHub é dividido em **3 colunas**.

- **span: 1** (Padrão): O botão ocupa 1/3 da largura da tela.
    
- **span: 2**: O botão ocupa 2/3 da largura (ideal para botões de Volume/Canal).
    
- **span: 3**: O botão ocupa a largura total da linha (ideal para o botão de Power no topo).
    
- **rowSpan: 2**: O botão ocupa duas linhas de altura (ideal para criar botões verticais).
    

---

### 4. Criando Espaçamentos

Se você quiser deixar um espaço vazio entre dois botões para organizar melhor o layout, use o tipo `space`:

JSON

```
{ "name": "null", "type": "space", "span": 1 }
```

---

### 5. Exemplo Completo (Controle de TV)

Copie este código como base para começar o seu:

JSON

```
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

### 6. Como instalar o controle no dispositivo

1. Crie o arquivo no seu computador e salve como `remotes.json`.
    
2. Acesse a página inicial (Home) do seu **IRHub-8266**.
    
3. Vá até a seção **"Controles Remotos"**.
    
4. Clique em **"Escolher arquivo"** e selecione o seu `remotes.json`.
    
5. Clique em **"📤 Importar"**.
    
6. O novo modelo aparecerá instantaneamente no menu de seleção.
    

**Dica:** Use a aba **"IR"** do sistema para capturar os códigos do seu controle físico original. Basta apontar o controle para o IRHub, apertar a tecla e copiar o `Protocol`, `Code` e `Bits` que aparecerem no histórico.