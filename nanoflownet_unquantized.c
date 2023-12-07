
/*
 * Copyright (C) 2017 GreenWaves Technologies
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 *
 */

#include "stdio.h"

/* PMSIS includes. */
#include "pmsis.h"

/* PMSIS BSP includes. */
#include "bsp/bsp.h"
#include "bsp/camera.h"

/* Demo includes. */
#include "gaplib/ImgIO.h"

/* Autotiler includes. */
#include "nanoflownet_unquantized.h"
#include "nanoflownet_unquantizedKernels.h"

/* Flow processing includes. */
#include "flo_proc.h"

#ifdef __EMUL__
#define pmsis_exit(n) exit(n)
#endif

#ifndef STACK_SIZE
#define STACK_SIZE 1024
#endif

/* Camera defines */
#if defined(QVGA_IMG) /* QVGA */
    #define YRES 224
    #define XRES 162
#elif defined(QQVGA_IMG) /* QQVGA */
    #define YRES 162
    #define XRES 162
#else
    #define YRES 324
    #define XRES 324
#endif

#if defined(SLICE_MODE)
    #if defined(QVGA_IMG)
        #define ROI_WIDTH 162
        #define ROI_HEIGHT 112
        #define X (XRES - ROI_WIDTH) / 2
        #define Y (YRES - ROI_HEIGHT) / 2
        #define CAMERA_WIDTH ROI_WIDTH
        #define CAMERA_HEIGHT ROI_HEIGHT
    #elif defined(QQVGA_IMG)
        #define ROI_WIDTH 160
        #define ROI_HEIGHT 112
        #define X (XRES - ROI_WIDTH) / 2
        #define Y (YRES - ROI_HEIGHT) / 2
        #define CAMERA_WIDTH ROI_WIDTH
        #define CAMERA_HEIGHT ROI_HEIGHT
    #else
        #define ROI_WIDTH 162
        #define ROI_HEIGHT 112
        #define X (XRES - ROI_WIDTH) / 2
        #define Y (YRES - ROI_HEIGHT) / 2
        #define CAMERA_WIDTH ROI_WIDTH
        #define CAMERA_HEIGHT ROI_HEIGHT
    #endif
#endif

static struct pi_device cam;
static uint8_t *imgBuff0;
static uint8_t *imgBuff1;

struct pi_device uart;
struct pi_uart_conf uart_conf;

static struct pi_device gpio_device;
static int led_val = 0;

AT_HYPERFLASH_FS_EXT_ADDR_TYPE nanoflownet_unquantized_L3_Flash = 0;

/* Inputs */
static uint8_t *Input_1;
static uint8_t *Input_2;

static char imgName[50];
static uint32_t idx = 0;
static int32_t error;
static int8_t yaw_command;

/* Outputs */
static int8_t *Output_1;
static uint8_t *Output_2;

static int32_t open_camera_himax(struct pi_device *device)
{
    struct pi_himax_conf cam_conf;
    pi_himax_conf_init(&cam_conf);

#ifdef SLICE_MODE
    cam_conf.roi.slice_en = 1;
    cam_conf.roi.x = X;
    cam_conf.roi.y = Y;
    cam_conf.roi.w = CAMERA_WIDTH;
    cam_conf.roi.h = CAMERA_HEIGHT;
#endif

#ifdef QQVGA_IMG
    cam_conf.format = PI_CAMERA_QQVGA;
#endif

    pi_open_from_conf(device, &cam_conf);
    if (pi_camera_open(device))
    {
        return -1;
    }

    // rotate image
    pi_camera_control(&cam, PI_CAMERA_CMD_START, 0);
    uint8_t set_value = 3;
    uint8_t reg_value;
    pi_camera_reg_set(&cam, IMG_ORIENTATION, &set_value);
    pi_time_wait_us(1000000);
    pi_camera_reg_get(&cam, IMG_ORIENTATION, &reg_value);
    if (set_value != reg_value)
    {
        printf("Failed to rotate camera image\n");
        return -1;
    }

    pi_camera_control(&cam, PI_CAMERA_CMD_STOP, 0);

    /* Let the camera AEG work for 100ms */
    pi_camera_control(&cam, PI_CAMERA_CMD_AEG_INIT, 0);

    return 0;
}

static void cluster()
{
    // #ifdef PERF
    //     printf("Start timer\n");
    //     gap_cl_starttimer();
    //     gap_cl_resethwtimer();
    // #endif

    nanoflownet_unquantizedCNN(Input_1, Input_2, Output_1);
    printf("Runner completed\n");
}

int test_nanoflownet_unquantized(void)
{
    printf("Entering main controller\n");
    /* ---------------->
     * Put here Your input settings
     * <---------------
     */

#ifndef __EMUL__
    /* Configure And open cluster. */
    struct pi_device cluster_dev;
    struct pi_cluster_conf cl_conf;
    cl_conf.id = 0;
    pi_open_from_conf(&cluster_dev, (void *)&cl_conf);
    if (pi_cluster_open(&cluster_dev))
    {
        printf("Cluster open failed !\n");
        pmsis_exit(-4);
    }

    /* Frequency Settings: defined in the Makefile */
    int cur_fc_freq = pi_freq_set(PI_FREQ_DOMAIN_FC, FREQ_FC * 1000 * 1000);
    int cur_cl_freq = pi_freq_set(PI_FREQ_DOMAIN_CL, FREQ_CL * 1000 * 1000);
    int cur_pe_freq = pi_freq_set(PI_FREQ_DOMAIN_PERIPH, FREQ_PE * 1000 * 1000);
    if (cur_fc_freq == -1 || cur_cl_freq == -1 || cur_pe_freq == -1)
    {
        printf("Error changing frequency !\nTest failed...\n");
        pmsis_exit(-4);
    }
    printf("FC Frequency as %d Hz, CL Frequency = %d Hz, PERIIPH Frequency = %d Hz\n",
           pi_freq_get(PI_FREQ_DOMAIN_FC), pi_freq_get(PI_FREQ_DOMAIN_CL), pi_freq_get(PI_FREQ_DOMAIN_PERIPH));

    if (PMU_set_voltage(1200, 0))
    {
        printf("Failed to set voltage\n");
        pmsis_exit(-4);
    }
    printf("Set voltage to 1.2V\n");
#endif

    /* Open camera */
    if (open_camera_himax(&cam))
    {
        printf("Failed to open camera\n");
        pmsis_exit(-2);
    }
    printf("Camera opened\n");

    printf("Will capture images at %d x %d resolution\n", CAMERA_WIDTH, CAMERA_HEIGHT);

    imgBuff0 = (uint8_t *)pmsis_l2_malloc((CAMERA_WIDTH * CAMERA_HEIGHT) * sizeof(uint8_t));
    if (imgBuff0 == NULL)
    {
        printf("Failed to allocate Memory for Image, asking for: %d * %d\n", CAMERA_WIDTH, CAMERA_HEIGHT);
        pmsis_exit(-1);
    }

    imgBuff1 = (uint8_t *)pmsis_l2_malloc((CAMERA_WIDTH * CAMERA_HEIGHT) * sizeof(uint8_t));
    if (imgBuff1 == NULL)
    {
        printf("Failed to allocate Memory for Image, asking for: %d * %d\n", CAMERA_WIDTH, CAMERA_HEIGHT);
        pmsis_exit(-1);
    }

    Output_1 = (int8_t *)pmsis_l2_malloc((40 * 28 * 2) * sizeof(int8_t));
    if (Output_1 == NULL)
    {
        printf("Failed to allocate Memory for Image, asking for: %d * %d * %d\n", 40, 28, 2);
        pmsis_exit(-1);
    }

    Output_2 = (uint8_t *)pmsis_l2_malloc((40 * 28) * sizeof(uint8_t));
    if (Output_2 == NULL)
    {
        printf("Failed to allocate Memory for Image, asking for: %d * %d\n", 40, 28);
        pmsis_exit(-1);
    }

    /* Init and open UART */
    pi_uart_conf_init(&uart_conf);
    uart_conf.enable_tx = 1;
    uart_conf.enable_rx = 1;
    uart_conf.baudrate_bps = 115200;
    pi_open_from_conf(&uart, &uart_conf);
    if (pi_uart_open(&uart))
    {
        printf("UART failed to open !\n");
        pmsis_exit(-1);
    }

    // IMPORTANT - MUST BE CALLED AFTER THE CLUSTER IS SWITCHED ON!!!!
    printf("Constructor\n");
    int ConstructorErr = nanoflownet_unquantizedCNN_Construct();
    if (ConstructorErr)
    {
        printf("Graph constructor exited with error: %d\n(check the generated file nanoflownet_unquantizedKernels.c to see which memory have failed to be allocated)\n", ConstructorErr);
        pmsis_exit(-6);
    }

    printf("Call cluster\n");
#ifndef __EMUL__
    struct pi_cluster_task task;
    pi_cluster_task(&task, NULL, NULL);
    task.entry = cluster;
    task.arg = NULL;
    task.stack_size = (unsigned int)STACK_SIZE;
    task.slave_stack_size = (unsigned int)SLAVE_STACK_SIZE;
#endif

    Input_1 = imgBuff0;
    Input_2 = imgBuff1;

    pi_gpio_pin_configure(&gpio_device, 2, PI_GPIO_OUTPUT);

    while (1)
    {
        pi_gpio_pin_write(&gpio_device, 2, led_val);
        led_val ^= 1;

#ifndef __EMUL__
        pi_cluster_send_task_to_cl(&cluster_dev, &task);
#else
        cluster();
#endif

        // Switch pointers, avoid memcpy
        uint8_t *temp = Input_1;
        Input_1 = Input_2;
        Input_2 = temp;

        if (pi_camera_control(&cam, PI_CAMERA_CMD_START, 0))
        {
            printf("Failed to start camera\n");
            pmsis_exit(-3);
        }
        pi_camera_capture(&cam, Input_1, CAMERA_WIDTH * CAMERA_HEIGHT); // assume Input_1 is always new image
        if (pi_camera_control(&cam, PI_CAMERA_CMD_STOP, 0))
        {
            printf("Failed to stop camera\n");
            pmsis_exit(-3);
        }
        // sprintf(imgName, "../../../out/img_OUT_%ld_a.ppm", idx);
        // WriteImageToFile(imgName, CAMERA_WIDTH, CAMERA_HEIGHT, sizeof(uint8_t), Input_2, GRAY_SCALE_IO);
        // sprintf(imgName, "../../../out/img_OUT_%ld_b.ppm", idx);
        // WriteImageToFile(imgName, CAMERA_WIDTH, CAMERA_HEIGHT, sizeof(uint8_t), Input_1, GRAY_SCALE_IO);

        error = error * 9 / 10 + (flow_error(Output_1, Output_2, 40, 28)) / 10;
        printf("Error is %d\n", error);
        yaw_command =error/30; //800
        printf("Yaw command is %d\n", yaw_command);
        sprintf(imgName, "../../../out/floMagnitude_%ld.ppm", idx);
        // WriteImageToFile(imgName, 40, 28, sizeof(uint8_t), Output_2, GRAY_SCALE_IO);
        // printf("Flow error: %d\n", error);ma
        idx++;
        pi_uart_write(&uart, &yaw_command, 1);
    }

    nanoflownet_unquantizedCNN_Destruct();

#ifdef PERF
    {
        unsigned int TotalCycles = 0, TotalOper = 0;
        printf("\n");
        for (unsigned int i = 0; i < (sizeof(AT_GraphPerf) / sizeof(unsigned int)); i++)
        {
            printf("%45s: Cycles: %10u, Operations: %10u, Operations/Cycle: %f\n", AT_GraphNodeNames[i], AT_GraphPerf[i], AT_GraphOperInfosNames[i], ((float)AT_GraphOperInfosNames[i]) / AT_GraphPerf[i]);
            TotalCycles += AT_GraphPerf[i];
            TotalOper += AT_GraphOperInfosNames[i];
        }
        printf("\n");
        printf("%45s: Cycles: %10u, Operations: %10u, Operations/Cycle: %f\n", "Total", TotalCycles, TotalOper, ((float)TotalOper) / TotalCycles);
        printf("\n");
    }
#endif

    printf("Ended\n");
    pmsis_exit(0);
    return 0;
}

int main(int argc, char *argv[])
{
    printf("\n\n\t *** NNTOOL nanoflownet_unquantized Example ***\n\n");
#ifdef __EMUL__
    test_nanoflownet_unquantized();
#else
    return pmsis_kickoff((void *)test_nanoflownet_unquantized);
#endif
    return 0;
}
