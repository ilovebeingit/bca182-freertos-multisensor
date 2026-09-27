# Diagrams

Mermaid sources for the three design diagrams. Each `.mmd` file is the source
of truth, drawn from the current code; its header comment names the files it
was taken from.

| Source | Rendered | Shows |
|---|---|---|
| `architecture.mmd` | `architecture.png` | STM32 Blue Pill, the peripherals and their pin connections (from `diagram.json` and the `*_init()` functions) |
| `task-communication.mmd` | `task-communication.png` | The six FreeRTOS tasks, the queues, queue set and event group between them, and `serialMutex` |
| `state-machine.mmd` | `state-machine.png` | The ACTIVE / INACTIVE state machine, the 15 s timeout and what each state does |

The PNGs were rendered from the sources with the command below. After editing
a `.mmd` file, re-export its PNG so the two stay in sync.

## Rendering and exporting

- **Online:** paste a file's contents into <https://mermaid.live> and export
  PNG or SVG from there.
- **Command line** (Node.js required):

  ```sh
  npx -y @mermaid-js/mermaid-cli -i docs/diagrams/architecture.mmd -o docs/diagrams/architecture.png -s 2 -b white
  ```

  Use `-o name.svg` for a vector image.
