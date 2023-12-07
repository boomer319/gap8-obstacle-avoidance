# gap8-obstacle-avoidance
This repository contains the code used to deploy NanoFlowNet on the AI-deck.

Please refer to the [main repository](https://github.com/tudelft/nanoflownet) for the used models and the Crazyflie obstacle avoidance application

---

## How to use

### Install `gap_sdk`
```
git clone git@github.com:GreenWaves-Technologies/gap_sdk.git
git checkout ef8ee923dfb39cf2ef962d25e5bf69f8881ebfb3
```

Implement the following bug workaround in `gap_sdk`. Modify `rtos/pmsis/bsp/include/bsp/ai_deck.h`, put the following at the bottom of the file, before the `#endif`
```c
// #define CONFIG_SPIFLASH_SECTOR_SIZE (1<<12)

#include "bsp/flash/hyperflash.h"
#include "bsp/ram/hyperram.h"

#define pi_default_flash_conf pi_hyperflash_conf
#define pi_default_flash_conf_init pi_hyperflash_conf_init

#define pi_default_ram_conf pi_hyperram_conf
#define pi_default_ram_conf_init pi_hyperram_conf_init

// #endif
```

Follow the installation [instructions](https://github.com/GreenWaves-Technologies/gap_sdk/tree/ef8ee923dfb39cf2ef962d25e5bf69f8881ebfb3) (README.md, make sure you are on the suggested commit). Source the AI-deck config file; `configs/ai_deck.sh`. Modify `OPENOCD_CABLE` if necessary (redo source). Perform a full install; `make sdk`.

### Configuration
`nntool_script` can be modified to include grayscale image (112hx160w) file paths for quantization and quantization evaluation. No sample images are included. Default config uses fquant ("fake" quantization).

### Execution
To execute on the AI-deck, in this repo:
```
make all run PMSIS_OS=pulpos platform=board
```

To flash
```
make all flash PMSIS_OS=pulpos platform=board
```

`freertos` and `gvsoc` are not supported.

## Publication:
[arXiv preprint](https://arxiv.org/abs/2209.06918)

[IEEE-ICRA 2023 paper](https://ieeexplore.ieee.org/document/10161258)

Please cite us as follows:
```
@inproceedings{bouwmeester2023nanoflownet,
  title={Nanoflownet: Real-time dense optical flow on a nano quadcopter},
  author={Bouwmeester, Rik J and Paredes-Vall{\'e}s, Federico and De Croon, Guido CHE},
  booktitle={2023 IEEE International Conference on Robotics and Automation (ICRA)},
  pages={1996--2003},
  year={2023},
  organization={IEEE}
}
```
