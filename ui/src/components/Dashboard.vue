<template>
  <div class="py-6">
    <div class="mx-auto max-w-7xl px-4 sm:px-6 md:px-8">
      <h1 class="text-2xl font-semibold text-gray-900">{{ $t("dashboard.dashboard") }}</h1>
    </div>
    <div class="mx-auto max-w-7xl px-4 sm:px-6 md:px-8">
      <div class="py-4">
        <div>
          <div>
            <h3 class="text-lg font-medium leading-6 text-gray-900">{{ $t("dashboard.lcd_display") }}</h3>
            <LCD :line1="LCDStore.LCDTextLines[0]" :line2="LCDStore.LCDTextLines[1]" :line3="LCDStore.LCDTextLines[2]" :line4="LCDStore.LCDTextLines[3]" />
          </div>
          <TempControlDashPanel />
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import { useLCDStore } from "@/stores/LCDStore";
import { useTempControlStore } from "@/stores/TempControlStore";
import LCD from "@/components/dashboard/LCD.vue";
import TempControlDashPanel from "@/components/dashboard/TempControlDashPanel.vue";

export default {
  name: "Dashboard",
  components: {
    TempControlDashPanel,
    LCD
  },
  data() {
    return {
      intervalObject: null,
    }
  },
  mounted() {
    // Retrieve initial data
    this.LCDStore.getLCD();

    // Set up periodic refreshes
    this.intervalObject = window.setInterval(() => {
      this.LCDStore.getLCD();
    }, 5000)
  },
  beforeUnmount() {
    clearInterval(this.intervalObject);
  },
  setup() {
    return {
      LCDStore: useLCDStore(),  // Updated in Dashboard.vue
      TempControlStore: useTempControlStore()  // Updated in App.vue
    }
  },

}
</script>

<style scoped>

</style>