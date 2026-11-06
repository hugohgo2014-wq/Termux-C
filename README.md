# Terminal

Projeto Android nativo chamado **Terminal**, com interface em C + Android NDK + OpenGL ES 3.0.

## Estado atual

Este repositório começa com a base de compilação Android e uma Activity nativa renderizada em OpenGL ES 3.0. Ainda **não** é um port completo do Termux nem inclui o servidor Termux:X11. Esses componentes têm arquitetura, dependências e licenças próprias; a integração real será feita por etapas, sem apresentar uma aba X11 falsa como funcional.

## Compilar

O workflow **Build Terminal APK** compila o APK de debug para ARM64 e x86_64 usando Android NDK e Gradle.

## Arquitetura planejada

- Núcleo e interface: C / Android NDK.
- Renderização: EGL + OpenGL ES 3.0, com visual pixelado.
- Shell e ambiente Linux: integração progressiva com o bootstrap e os pacotes oficiais do Termux.
- Aba X11: integração do servidor Termux:X11 e do pacote auxiliar compatível.
- Permissões Android: solicitadas sob demanda e apenas quando o recurso correspondente precisar delas.

## Referências upstream

- https://github.com/termux/termux-app
- https://github.com/termux/termux-packages
- https://github.com/termux/termux-x11

## Licenças

O projeto deve preservar avisos de copyright e licenças de cada componente incorporado. Termux:X11 é GPL-3.0.
