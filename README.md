# Terminal

Aplicativo Android nativo com núcleo em C, Android NDK e renderização OpenGL ES 3.0.

## Objetivo

Criar um terminal Android independente, com uma interface nativa pixelada e uma área X11 integrada à navegação do app. O nome do aplicativo é **Terminal**.

## Estado real do código

Esta branch contém a base NativeActivity em C e EGL/OpenGL ES 3.0, além do workflow de compilação Android. A tela C atual ainda é uma demonstração gráfica: **não é um shell Linux funcional, não inclui o bootstrap do Termux e ainda não contém o servidor Termux:X11**. Não instale esta versão esperando ter todas as funções do Termux.

O desenvolvimento deve avançar por componentes verificáveis:
1. Runtime de terminal C com PTY, entrada/saída, teclado Android e sessões persistentes.
2. Bootstrap oficial Termux e integração com os repositórios de pacotes, respeitando ABI, prefixo e licenças.
3. Aba X11 ligada ao servidor real Termux:X11 e ao pacote auxiliar correspondente.
4. Permissões Android solicitadas no momento de uso, com explicação e fluxo de autorização.
5. Testes em dispositivo/emulador ARM64 e validação do APK gerado.

## Linguagem e arquitetura

- Código próprio de interface e runtime: C / Android NDK.
- Renderização: EGL + OpenGL ES 3.0.
- Activity: Android NativeActivity.
- Gradle é usado somente para empacotar e compilar o APK.
- Não usar WebView/HTML.
- Recursos Android que dependem de APIs de framework devem ser acessados pela interface nativa/NDK; permissões privilegiadas não podem ser concedidas por um app comum.

## X11: requisito upstream

Termux:X11 oficial não é uma biblioteca que possa ser simplesmente desenhada dentro de uma aba. O upstream descreve um aplicativo Android servidor X11 **e** um pacote companheiro no ambiente Termux. A integração precisa fornecer os dois e respeitar a compatibilidade de assinatura quando usar shared UID. A aba X11 só deve ser marcada como funcional depois de iniciar uma sessão X real.

## Permissões

O app pode declarar e solicitar permissões normais/perigosas compatíveis com sua versão Android. Acesso especial (por exemplo, gerenciamento amplo de arquivos, sobreposição e ignorar otimização de bateria) exige telas de configuração próprias. Permissões de sistema/assinatura, acesso irrestrito ao sistema e root não podem ser obtidos automaticamente.

## Compilar

O workflow **Build Terminal APK** em `.github/workflows/android.yml` compila o APK debug para `arm64-v8a` e `x86_64` e publica o artefato do GitHub Actions.

## Referências upstream

- Termux app: https://github.com/termux/termux-app
- Termux packages/bootstrap: https://github.com/termux/termux-packages
- Termux:X11: https://github.com/termux/termux-x11

## Licenças

Não copie código upstream sem preservar a licença e os avisos de copyright. Termux:X11 é GPL-3.0. Verifique a licença de cada componente antes de incorporar ou redistribuir binários.
