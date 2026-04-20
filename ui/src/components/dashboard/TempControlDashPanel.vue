<template>
  <div>
    <h3 class="text-lg font-medium leading-6 text-gray-900">{{ $t("dashboard.tc_dash.current_temps_header") }}</h3>
    <dl class="mt-5 grid grid-cols-1 gap-5 sm:grid-cols-3">
      <div class="overflow-hidden rounded-lg bg-white px-4 py-5 shadow sm:p-6">
        <dt class="truncate text-sm font-medium text-gray-500">{{ $t("dashboard.tc_dash.fridge_temp_header") }}</dt>
        <dd class="mt-1 text-3xl font-semibold tracking-tight text-gray-900" v-if="TempControlStore.hasTempInfo">
          <span v-html="formatTemp(TempControlStore.fridgeTemp)"></span>
        </dd>
        <dd class="mt-1 text-3xl font-semibold tracking-tight text-gray-900" v-else>--&deg; --</dd>
      </div>
      <div class="overflow-hidden rounded-lg bg-white px-4 py-5 shadow sm:p-6">
        <dt class="truncate text-sm font-medium text-gray-500">{{ $t("dashboard.tc_dash.beer_temp_header") }}</dt>
        <dd class="mt-1 text-3xl font-semibold tracking-tight text-gray-900" v-if="TempControlStore.hasTempInfo"><span v-html="formatTemp(TempControlStore.beerTemp)"></span></dd>
        <dd class="mt-1 text-3xl font-semibold tracking-tight text-gray-900" v-else>--&deg; --</dd>
      </div>
      <div class="overflow-hidden rounded-lg bg-white px-4 py-5 shadow sm:p-6">
        <dt class="truncate text-sm font-medium text-gray-500">{{ $t("dashboard.tc_dash.room_temp_header") }}</dt>
        <dd class="mt-1 text-3xl font-semibold tracking-tight text-gray-900" v-if="TempControlStore.hasTempInfo"><span v-html="formatTemp(TempControlStore.roomTemp)"></span></dd>
        <dd class="mt-1 text-3xl font-semibold tracking-tight text-gray-900" v-else>--&deg; --</dd>
      </div>
      <div class="overflow-hidden rounded-lg bg-white px-4 py-5 shadow sm:p-6">
        <dt class="truncate text-sm font-medium text-gray-500">{{ $t("dashboard.tc_dash.control_mode_header") }}</dt>
        <dd class="mt-1 text-3xl font-semibold tracking-tight text-gray-900 flex items-baseline justify-between md:block lg:flex" v-if="TempControlStore.hasTempInfo">
          <span v-if="TempControlStore.controlMode === 'f'" class="flex items-baseline">{{ $t("sitewide.brewpi_modes.fridge_constant") }}</span>
          <span v-else-if="TempControlStore.controlMode === 'b'">{{ $t("sitewide.brewpi_modes.beer_constant") }}</span>
          <span v-else-if="TempControlStore.controlMode === 'p'">{{ $t("sitewide.brewpi_modes.beer_profile") }}</span>
          <span v-else-if="TempControlStore.controlMode === 'o'">{{ $t("sitewide.brewpi_modes.off") }}</span>
          <span v-else-if="TempControlStore.controlMode === 't'">{{ $t("sitewide.brewpi_modes.test") }}</span>
          <div class=" text-green-800 inline-flex items-baseline px-2.5 py-0.5 rounded-full text-sm font-medium md:mt-2 lg:mt-0">
<!--            <InformationCircleIcon class="-ml-1 mr-0.5 h-5 w-5 flex-shrink-0 self-center text-green-500" aria-hidden="true" />-->
<!--            Change Mode-->
            <ModeChangeModal />
          </div>
        </dd>
        <dd class="mt-1 text-3xl font-semibold tracking-tight text-gray-900" v-else>--</dd>
      </div>
      <div class="overflow-hidden rounded-lg bg-white px-4 py-5 shadow sm:p-6">
        <dt class="truncate text-sm font-medium text-gray-500">Control Status</dt>
        <dd class="mt-1 text-3xl font-semibold tracking-tight text-gray-900" v-if="TempControlStore.hasTempInfo">
          <span v-if="TempControlStore.controlState === 0">{{ $t("sitewide.control_state.idle") }}</span>
          <span v-else-if="TempControlStore.controlState === 1">{{ $t("sitewide.brewpi_modes.off") }}</span>
          <span v-else-if="TempControlStore.controlState === 2">{{ $t("sitewide.control_state.door_open") }}</span>
          <span v-else-if="TempControlStore.controlState === 3">{{ $t("sitewide.control_state.heating") }}</span>
          <span v-else-if="TempControlStore.controlState === 4">{{ $t("sitewide.control_state.cooling") }}</span>
          <span v-else-if="TempControlStore.controlState === 5">{{ $t("sitewide.control_state.waiting_to_cool") }}</span>
          <span v-else-if="TempControlStore.controlState === 6">{{ $t("sitewide.control_state.waiting_to_heat") }}</span>
          <span v-else-if="TempControlStore.controlState === 7">{{ $t("sitewide.control_state.waiting_for_peak") }}</span>
          <span v-else-if="TempControlStore.controlState === 8">{{ $t("sitewide.control_state.min_cool_time") }}</span>
          <span v-else-if="TempControlStore.controlState === 9">{{ $t("sitewide.control_state.min_heat_time") }}</span>
        </dd>
        <dd class="mt-1 text-3xl font-semibold tracking-tight text-gray-900" v-else>--</dd>
      </div>
    </dl>
  </div>
</template>

<script setup>
import { useTempControlStore } from "@/stores/TempControlStore";
import { ExclamationTriangleIcon, InformationCircleIcon } from '@heroicons/vue/24/outline'
import ModeChangeModal from "@/components/dashboard/ModeChangeModal.vue";

const TempControlStore = useTempControlStore();

function formatTemp(temp) {
      const temp_num = Number.parseFloat(temp);
  if (isNaN(temp_num)) return "--" + "&deg; " + TempControlStore.cc.tempFormat;
  return temp_num.toFixed(1) + "&deg; " + TempControlStore.cc.tempFormat;
}
</script>

<style scoped>

</style>