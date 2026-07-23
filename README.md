# Tavern

## Tavern is a tavern simulator written in C and uses ncurses.

<p align="center">
	<img src="assets/screenshot.jpg" width="822">
</p>

It Features:

- Rent
- Stock management
- Price management
- Reputation management
- Quality management
- Ale and Wine
- Rumors
- Consistency 
- Handsomeness (affects reputation)
- Supplier and supplier price instability
- Awesome ncurses UI
- Dirty pathways (causes customers to fall, you have to clean it!)

Here's a to-do list for this game (The ones with strikethrough are done):

- ~~Inflation~~
- ~~Customers doing weird stuff that you have to take care of~~
- ~~Bad things happening to your stocks, like thieves stealing them~~
- ~~Changes in population, so the potential customer number is not stable.~~
- Make code compatible with c89... maybe?
- Maybe the kingdom you're in could start wars and that could affect economy and your customers, etc.
- *and much more stuff that could happen in an actual medieval tavern...*

## itch.io
Please comment on the [itch.io page](https://terra2o.itch.io/tavern)! You can also get pre-built binaries there for macOS, Linux, and Windows.


## Build
### For building on older systems, check out [COMPILING.md](./COMPILING.md).
### Needs:
- GNU Make
- gcc
- ncurses/PDcurses

run `make release` in root directory of the project (where Makefile is located)
