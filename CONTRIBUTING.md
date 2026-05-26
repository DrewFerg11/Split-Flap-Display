# Contributing

Thanks for your interest! This project is a community fork of
[jhoff/Split-Flap-Display](https://github.com/jhoff/Split-Flap-Display), which
is itself based on the original design by
[Morgan Manly](https://github.com/ManlyMorgan/Split-Flap-Display).

## Reporting issues

- **Bugs:** Open a [GitHub issue](../../issues/new) with the board you're using,
  what you expected, what actually happened, and (if relevant) the serial log.
- **Feature ideas:** Issues are fine, but if it's open-ended, the
  [Discord](https://discord.gg/RCvks4XXXH) is usually a better place to discuss
  first.
- **Questions / help:** Try the Discord — issues are best reserved for things
  that are actionable in code.

## Pull requests

1. Fork the repo and create a branch off `main` with a descriptive name.
2. Make your change. Keep it focused — one logical change per PR.
3. Run the formatter before committing:
   ```
   npm run format
   ```
   The `format-check` workflow will fail your PR if this hasn't been run.
4. Push to your fork and open a PR against `main` here.
5. Be patient — this is a side project; reviews may take a few days.

For build setup, see the [Setup Instructions](README.md#setup-instructions) in
the README.

## A note on licensing

Neither this fork nor its upstream currently has an explicit software license,
which means the firmware code is — strictly — under default copyright by its
original authors. By opening a PR you agree that your contribution may later be
relicensed under whatever permissive open-source license the project eventually
adopts (most likely MIT). If that's a dealbreaker for you, please raise it in
an issue first.

The 3D model files are covered separately by
[LICENSE-HARDWARE.md](LICENSE-HARDWARE.md) (CC BY-NC-SA 4.0).
