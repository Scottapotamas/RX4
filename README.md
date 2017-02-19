# RX4
Four receiver diversity board for 5.8Ghz video. Features onboard regulator, buzzer, oled and more. Should fit in a compact form factor.

I need to get the software working properly. I've tried to leave the source available where possible, though I haven't documented much of this as its a personal project.

# Progress so far

Some enclosure concepts have been drawn up.

![Proposed Enclosure](https://raw.githubusercontent.com/Scottapotamas/RX4/master/Enclosure%20Render.jpg)

The board has been made, and a basic level of validation done to show that everything works hardware wise.

![3D Iso View](https://raw.githubusercontent.com/Scottapotamas/RX4/master/3D%20Iso.PNG)

![3D Top View](https://github.com/Scottapotamas/RX4/blob/master/3D%20Top.PNG)

The schematics and gerbers are all in this repo if anyone wants to borrow ideas for similar designs. There are other boards out there like the RX5800-Pro project and similar, but we use the TX5813 modules on this board due to native SPI support (no need to mod the 5800 internal resistors).

![Top Layer](https://raw.githubusercontent.com/Scottapotamas/RX4/master/Top%20Layer.png)

![Bottom Layer](https://raw.githubusercontent.com/Scottapotamas/RX4/master/Bottom%20Layer.png)

The board is a simple 2-layer, 1oz design that fits inside a 80x80mm footprint. It should cost less than $40 to produce if you buy parts from Aliexpress/Banggood/Chinese sellers.
