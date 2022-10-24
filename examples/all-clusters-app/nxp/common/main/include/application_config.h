/*
 *
 *    Copyright (c) 2021 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once


// ---- All cluster Example App Config ----

#define RESET_BUTTON 1
#define CLUSTER_BUTTON 2
#define JOIN_BUTTON 3
#define TCPIP_BUTTON 4

#define RESET_BUTTON_PUSH 1
#define CLUSTER_BUTTON_PUSH 2
#define JOIN_BUTTON_PUSH 3
#define TCPIP_BUTTON_PUSH 4

#define APP_BUTTON_PUSH 1

// Time it takes for the light to switch on/off
#define ACTUATOR_MOVEMENT_PERIOS_MS 50

// ---- All Cluster Example SWU Config ----
#define SWU_INTERVAl_WINDOW_MIN_MS (23 * 60 * 60 * 1000) // 23 hours
#define SWU_INTERVAl_WINDOW_MAX_MS (24 * 60 * 60 * 1000) // 24 hours
