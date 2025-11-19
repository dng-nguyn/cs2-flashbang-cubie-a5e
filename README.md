# cs2-flashbang-cubie-a5e

Low latency flashbang amplifier.

Libraries used: [cs2mqtt](https://github.com/lupusbytes/cs2mqtt)

you need to modify credentials before using the script

this script uses a cubie a5e and a relay module to toggle a very big led for flashbang funny effect.

you will need to setup cs2mqtt server with an appropriate mqtt broker, as well as configure your cs2 game state integration.

data required is ` "player_state"        "1"` and `"provider"            "1" ` for minimal overhead

default pin used is pin 3, which corresponds to index 37 because reasons. refer to [radxa docs](https://docs.radxa.com/en/cubie/a5e/hardware-use/pin-gpio) to find your appropriate pin

<img width="603" height="399" alt="{B21C10E2-E47A-4F49-82F1-9D0464E90014}" src="https://github.com/user-attachments/assets/16671334-a6df-4ac3-8032-441f3dcb5bc7" />


<img width="288" height="533" alt="{D859D725-DBF0-4BDD-B188-1B8940374A6A}" src="https://github.com/user-attachments/assets/22b12928-665f-4c0e-8fef-8eefdfbf790f" />
