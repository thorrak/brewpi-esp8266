<template>
  <div class="max-w-7xl mx-auto py-6 sm:px-6 lg:px-8">
    <div class="flex-initial md:container">
      <div class="bg-white overflow-hidden sm:rounded-lg sm:shadow">

        <div class="bg-white px-4 py-5 border-b border-gray-200 sm:px-6">
          <h3 class="text-lg leading-6 font-medium text-gray-900">
            {{ $t("about.uptime.version_and_uptime") }}
          </h3>
        </div>

        <div class="flex flex-col">
          <div class="-my-2 overflow-x-auto sm:-mx-6 lg:-mx-8">
            <div class="py-2 align-middle inline-block min-w-full sm:px-6 lg:px-8">
              <div class="shadow overflow-hidden border-b border-gray-200 sm:rounded-lg">
                <table class="min-w-full divide-y divide-gray-200">
                  <tbody class="bg-white divide-y divide-gray-200">
                  <tr>
                    <th scope="col" class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      {{ $t("about.uptime.firmware_version") }}
                    </th>
                    <td class="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900">
                      BrewPi-ESP {{ VersionInfoStore.brewpiespVersion }} ({{ VersionInfoStore.gitRevision }})
                    </td>
                  </tr>
                  <tr>
                    <th scope="col" class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      {{ $t("about.uptime.uptime_header") }}
                    </th>
                    <td class="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900">
                      {{ $t('about.uptime.uptime_print', { days: UptimeStatsStore.days, hours: UptimeStatsStore.hours, minutes: UptimeStatsStore.minutes, seconds: UptimeStatsStore.seconds}) }}
                    </td>
                  </tr>
                  <tr>
                    <th scope="col" class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      {{ $t("about.uptime.reset_header") }}
                    </th>
                    <td class="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900">
                      {{ $t('about.uptime.reset_print', { reason: ResetReasonStore.reason, description: ResetReasonStore.description}) }}
                    </td>
                  </tr>
                  <tr v-if="HeapInfoStore.hasHeapInfo">
                    <th scope="col" class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      {{ $t("about.uptime.heap_header") }}
                    </th>
                    <td class="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900">
                      {{ $t('about.uptime.heap_print', { free_heap: HeapInfoStore.free, max_heap: HeapInfoStore.max, frags: HeapInfoStore.frag }) }}
                    </td>
                  </tr>

                  </tbody>
                </table>
              </div>
            </div>
          </div>
        </div>


      </div>
    </div>
  </div>

  <!-- Controller Actions Section -->
  <div class="max-w-7xl mx-auto py-6 sm:px-6 lg:px-8">
    <div class="flex-initial md:container">
      <div class="bg-white overflow-hidden sm:rounded-lg sm:shadow">

        <div class="bg-white px-4 py-5 border-b border-gray-200 sm:px-6">
          <h3 class="text-lg leading-6 font-medium text-gray-900">
            {{ $t("about.controller_actions.header") }}
          </h3>
          <p class="mt-1 text-sm text-gray-500">
            {{ $t("about.controller_actions.description") }}
          </p>
        </div>

        <div class="px-4 py-5 sm:p-6">
          <!-- Warning Banner -->
          <div class="border-l-4 border-yellow-400 bg-yellow-50 p-4 mb-4">
            <div class="flex">
              <div class="flex-shrink-0">
                <ExclamationTriangleIcon class="h-5 w-5 text-yellow-400" aria-hidden="true" />
              </div>
              <div class="ml-3">
                <p class="text-sm text-yellow-700">
                  {{ $t("about.controller_actions.warning") }}
                </p>
              </div>
            </div>
          </div>

          <!-- Action Buttons Grid -->
          <div class="grid grid-cols-1 gap-4 sm:grid-cols-3">
            <!-- Restart Button -->
            <button @click="openConfirmModal('restart')" type="button"
              class="inline-flex items-center justify-center px-4 py-2 border border-transparent text-sm font-medium rounded-md shadow-sm text-white bg-blue-600 hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500">
              <ArrowPathIcon class="h-5 w-5 mr-2" aria-hidden="true" />
              {{ $t("about.controller_actions.restart_button") }}
            </button>

            <!-- Reset Connection Button -->
            <button @click="openConfirmModal('reset_connection')" type="button"
              class="inline-flex items-center justify-center px-4 py-2 border border-transparent text-sm font-medium rounded-md shadow-sm text-white bg-yellow-600 hover:bg-yellow-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-yellow-500">
              <WifiIcon class="h-5 w-5 mr-2" aria-hidden="true" />
              {{ $t("about.controller_actions.reset_connection_button") }}
            </button>

            <!-- Reset Config Button -->
            <button @click="openConfirmModal('reset_config')" type="button"
              class="inline-flex items-center justify-center px-4 py-2 border border-transparent text-sm font-medium rounded-md shadow-sm text-white bg-red-600 hover:bg-red-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-red-500">
              <TrashIcon class="h-5 w-5 mr-2" aria-hidden="true" />
              {{ $t("about.controller_actions.reset_config_button") }}
            </button>

            <!-- GLYCOL_DISABLED_ON_ESP_IDF START: Clear Glycol Log button
            <button @click="openConfirmModal('clear_glycol_log')" type="button"
              class="inline-flex items-center justify-center px-4 py-2 border border-transparent text-sm font-medium rounded-md shadow-sm text-white bg-slate-600 hover:bg-slate-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-slate-500">
              <ArchiveBoxXMarkIcon class="h-5 w-5 mr-2" aria-hidden="true" />
              {{ $t("about.controller_actions.clear_glycol_log_button") }}
            </button>
            GLYCOL_DISABLED_ON_ESP_IDF END -->
          </div>
        </div>

      </div>
    </div>
  </div>

  <!-- Confirmation Modal -->
  <TransitionRoot as="template" :show="confirmModalOpen">
    <Dialog as="div" class="fixed z-10 inset-0 overflow-y-auto" @close="confirmModalOpen = false">
      <div class="flex items-end justify-center min-h-screen pt-4 px-4 pb-20 text-center sm:block sm:p-0">
        <TransitionChild as="template" enter="ease-out duration-300" enter-from="opacity-0" enter-to="opacity-100" leave="ease-in duration-200" leave-from="opacity-100" leave-to="opacity-0">
          <DialogOverlay class="fixed inset-0 bg-gray-500 bg-opacity-75 transition-opacity" />
        </TransitionChild>

        <span class="hidden sm:inline-block sm:align-middle sm:h-screen" aria-hidden="true">&#8203;</span>

        <TransitionChild as="template" enter="ease-out duration-300" enter-from="opacity-0 translate-y-4 sm:translate-y-0 sm:scale-95" enter-to="opacity-100 translate-y-0 sm:scale-100" leave="ease-in duration-200" leave-from="opacity-100 translate-y-0 sm:scale-100" leave-to="opacity-0 translate-y-4 sm:translate-y-0 sm:scale-95">
          <div class="inline-block align-bottom bg-white rounded-lg text-left overflow-hidden shadow-xl transform transition-all sm:my-8 sm:align-middle sm:max-w-lg sm:w-full">
            <div class="bg-white px-4 pt-5 pb-4 sm:p-6 sm:pb-4">
              <div class="sm:flex sm:items-start">
                <div class="mx-auto flex-shrink-0 flex items-center justify-center h-12 w-12 rounded-full sm:mx-0 sm:h-10 sm:w-10"
                     :class="pendingAction === 'reset_config' ? 'bg-red-100' : 'bg-yellow-100'">
                  <ExclamationTriangleIcon class="h-6 w-6"
                    :class="pendingAction === 'reset_config' ? 'text-red-600' : 'text-yellow-600'"
                    aria-hidden="true" />
                </div>
                <div class="mt-3 text-center sm:mt-0 sm:ml-4 sm:text-left">
                  <DialogTitle as="h3" class="text-lg leading-6 font-medium text-gray-900">
                    {{ getConfirmTitle() }}
                  </DialogTitle>
                  <div class="mt-2">
                    <p class="text-sm text-gray-500">
                      {{ getConfirmMessage() }}
                    </p>
                  </div>
                </div>
              </div>
            </div>
            <div class="bg-gray-50 px-4 py-3 sm:px-6 sm:flex sm:flex-row-reverse">
              <button type="button" @click="executeAction()"
                class="w-full inline-flex justify-center rounded-md border border-transparent shadow-sm px-4 py-2 text-base font-medium text-white focus:outline-none focus:ring-2 focus:ring-offset-2 sm:ml-3 sm:w-auto sm:text-sm"
                :class="pendingAction === 'reset_config'
                  ? 'bg-red-600 hover:bg-red-700 focus:ring-red-500'
                  : 'bg-yellow-600 hover:bg-yellow-700 focus:ring-yellow-500'">
                {{ $t("about.controller_actions.confirm") }}
              </button>
              <button type="button" @click="confirmModalOpen = false"
                class="mt-3 w-full inline-flex justify-center rounded-md border border-gray-300 shadow-sm px-4 py-2 bg-white text-base font-medium text-gray-700 hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 sm:mt-0 sm:ml-3 sm:w-auto sm:text-sm">
                {{ $t("sitewide.cancel") }}
              </button>
            </div>
          </div>
        </TransitionChild>
      </div>
    </Dialog>
  </TransitionRoot>

  <!-- Result Modal -->
  <TransitionRoot as="template" :show="resultModalOpen">
    <Dialog as="div" class="fixed z-10 inset-0 overflow-y-auto" @close="closeResultModal()">
      <div class="flex items-end justify-center min-h-screen pt-4 px-4 pb-20 text-center sm:block sm:p-0">
        <TransitionChild as="template" enter="ease-out duration-300" enter-from="opacity-0" enter-to="opacity-100" leave="ease-in duration-200" leave-from="opacity-100" leave-to="opacity-0">
          <DialogOverlay class="fixed inset-0 bg-gray-500 bg-opacity-75 transition-opacity" />
        </TransitionChild>

        <span class="hidden sm:inline-block sm:align-middle sm:h-screen" aria-hidden="true">&#8203;</span>

        <TransitionChild as="template" enter="ease-out duration-300" enter-from="opacity-0 translate-y-4 sm:translate-y-0 sm:scale-95" enter-to="opacity-100 translate-y-0 sm:scale-100" leave="ease-in duration-200" leave-from="opacity-100 translate-y-0 sm:scale-100" leave-to="opacity-0 translate-y-4 sm:translate-y-0 sm:scale-95">
          <div class="inline-block align-bottom bg-white rounded-lg px-4 pt-5 pb-4 text-left overflow-hidden shadow-xl transform transition-all sm:my-8 sm:align-middle sm:max-w-sm sm:w-full sm:p-6">
            <div v-if="ControllerActionsStore.actionError">
              <div class="mx-auto flex items-center justify-center h-12 w-12 rounded-full bg-red-100">
                <NoSymbolIcon class="h-6 w-6 text-red-600" aria-hidden="true" />
              </div>
              <div class="mt-3 text-center sm:mt-5">
                <DialogTitle as="h3" class="text-lg leading-6 font-medium text-gray-900">
                  {{ $t("about.controller_actions.action_failed") }}
                </DialogTitle>
                <div class="mt-2">
                  <p class="text-sm text-gray-500">
                    {{ $t("about.controller_actions.action_failed_msg") }}
                  </p>
                </div>
              </div>
            </div>
            <div v-else>
              <div class="mx-auto flex items-center justify-center h-12 w-12 rounded-full bg-green-100">
                <CheckIcon class="h-6 w-6 text-green-600" aria-hidden="true" />
              </div>
              <div class="mt-3 text-center sm:mt-5">
                <DialogTitle as="h3" class="text-lg leading-6 font-medium text-gray-900">
                  {{ $t("about.controller_actions.action_triggered") }}
                </DialogTitle>
                <div class="mt-2">
                  <p class="text-sm text-gray-500">
                    {{ $t("about.controller_actions.action_triggered_msg") }}
                  </p>
                </div>
              </div>
            </div>

            <div class="mt-5 sm:mt-6">
              <button type="button" @click="closeResultModal()"
                class="inline-flex justify-center w-full rounded-md border border-transparent shadow-sm px-4 py-2 bg-indigo-600 text-base font-medium text-white hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 sm:text-sm">
                {{ $t("sitewide.close") }}
              </button>
            </div>
          </div>
        </TransitionChild>
      </div>
    </Dialog>
  </TransitionRoot>

</template>

<script setup>
import { ref, onMounted, onBeforeUnmount } from 'vue';
import {
  Dialog,
  DialogOverlay,
  DialogTitle,
  TransitionChild,
  TransitionRoot,
} from '@headlessui/vue';
import {
  CheckIcon,
  NoSymbolIcon,
  ExclamationTriangleIcon,
  ArrowPathIcon,
  WifiIcon,
  TrashIcon,
  // GLYCOL_DISABLED_ON_ESP_IDF: ArchiveBoxXMarkIcon, // used for clear_glycol_log button
} from '@heroicons/vue/24/outline';
import { useLoading } from "vue-loading-overlay";
import { useI18n } from 'vue-i18n';

// Existing store imports
import { useUptimeStatsStore } from "@/stores/UptimeStatsStore";
import { useVersionInfoStore } from "@/stores/VersionInfoStore.js";
import { useHeapInfoStore } from "@/stores/HeapInfoStore";
import { useResetReasonStore } from "@/stores/ResetReasonStore";
// New store import
import { useControllerActionsStore } from "@/stores/ControllerActionsStore";

// Initialize i18n
const { t } = useI18n();

// Store instances
const UptimeStatsStore = useUptimeStatsStore();
const VersionInfoStore = useVersionInfoStore();
const HeapInfoStore = useHeapInfoStore();
const ResetReasonStore = useResetReasonStore();
const ControllerActionsStore = useControllerActionsStore();

// Loading overlay
const $loading = useLoading({});

// Modal state
const confirmModalOpen = ref(false);
const resultModalOpen = ref(false);
const pendingAction = ref(null);  // 'restart' | 'reset_connection' | 'reset_config' | 'clear_glycol_log'

// Interval references
let statsIntervalObject = null;
let heapIntervalObject = null;
let resetIntervalObject = null;

// Lifecycle
onMounted(() => {
  // Retrieve initial data
  UptimeStatsStore.getUptimeStats();
  VersionInfoStore.getVersionInfo();
  HeapInfoStore.getHeapInfo();
  ResetReasonStore.getResetReason();

  // Set up periodic refreshes
  statsIntervalObject = window.setInterval(() => {
    UptimeStatsStore.getUptimeStats();
  }, 10000);
  heapIntervalObject = window.setInterval(() => {
    HeapInfoStore.getHeapInfo();
  }, 9000);
  resetIntervalObject = window.setInterval(() => {
    ResetReasonStore.getResetReason();
  }, 60000);
});

onBeforeUnmount(() => {
  clearInterval(statsIntervalObject);
  clearInterval(heapIntervalObject);
  clearInterval(resetIntervalObject);
});

// Modal functions
function openConfirmModal(action) {
  pendingAction.value = action;
  confirmModalOpen.value = true;
}

async function executeAction() {
  confirmModalOpen.value = false;
  const loader = $loading.show({});

  await ControllerActionsStore.performAction(pendingAction.value);

  loader.hide();
  resultModalOpen.value = true;
}

function closeResultModal() {
  resultModalOpen.value = false;
  ControllerActionsStore.clearError();
}

// Helper functions for dynamic content
function getConfirmTitle() {
  switch (pendingAction.value) {
    case 'restart':
      return t('about.controller_actions.confirm_restart_title');
    case 'reset_connection':
      return t('about.controller_actions.confirm_reset_connection_title');
    case 'reset_config':
      return t('about.controller_actions.confirm_reset_config_title');
    // GLYCOL_DISABLED_ON_ESP_IDF START: clear_glycol_log case
    // case 'clear_glycol_log':
    //   return t('about.controller_actions.confirm_clear_glycol_log_title');
    // GLYCOL_DISABLED_ON_ESP_IDF END
    default:
      return '';
  }
}

function getConfirmMessage() {
  switch (pendingAction.value) {
    case 'restart':
      return t('about.controller_actions.confirm_restart_msg');
    case 'reset_connection':
      return t('about.controller_actions.confirm_reset_connection_msg');
    case 'reset_config':
      return t('about.controller_actions.confirm_reset_config_msg');
    // GLYCOL_DISABLED_ON_ESP_IDF START: clear_glycol_log case
    // case 'clear_glycol_log':
    //   return t('about.controller_actions.confirm_clear_glycol_log_msg');
    // GLYCOL_DISABLED_ON_ESP_IDF END
    default:
      return '';
  }
}
</script>

<style scoped>

</style>