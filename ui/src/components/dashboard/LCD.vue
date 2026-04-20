<template>
  <div class="lcd-font" style="height: 125px; width: 290px">
    <div class="lcddisplay">
      <span class="lcd-text">
          <span class="lcd-line" id="lcd-line-0" v-html="cleanLCDText(line1)"></span>
          <span class="lcd-line" id="lcd-line-1" v-html="cleanLCDText(line2)"></span>
          <span class="lcd-line" id="lcd-line-2" v-html="cleanLCDText(line3)"></span>
          <span class="lcd-line" id="lcd-line-3" v-html="cleanLCDText(line4)"></span>
      </span>
    </div>
  </div>
</template>

<script>
import { useLCDStore } from "@/stores/LCDStore";

export default {
  name: "LCD",
  props: {
    line1: {
      type: String,
      required: true,
    },
    line2: {
      type: String,
      required: true,
    },
    line3: {
      type: String,
      required: true,
    },
    line4: {
      type: String,
      required: true,
    },
  },
  methods: {
    cleanLCDText(line) {
      let lineText = line;
      if (!(typeof lineText === 'string' || lineText instanceof String)) {
        lineText = "";
      }
      if (lineText.length > 20) {
        lineText = lineText.substring(0, 20);
      }
      while (lineText.length < 20) {
        lineText += " ";
      }
      for (let i = 0; i < lineText.length; i++) {
        // This will only work for the first degree symbol, due to the fact that we change the length of the string
        // I could spend time to figure out a better way to do this, but that's all I care about for this application.
        if (lineText.charCodeAt(i) > 127) {
          lineText = lineText.substring(0, i) + "&deg;" + lineText.substring(i + 1);
        }
      }
      lineText.replace(" ", "&nbsp;");
      return lineText;
    }
  },
  setup() {
    return {
      LCDStore: useLCDStore(),
    }
  },

}
</script>

<style scoped>

@font-face {
  font-family: '5x8LCD';
  src: url('@/assets/fonts/5x8_lcd.eot');
  src: url('@/assets/fonts/5x8_lcd.eot?#iefix') format('embedded-opentype'),
  url('@/assets/fonts/5x8_lcd.woff') format('woff');
  font-weight: normal;
  font-style: normal;
}

.lcd-font {
  font-family: '5x8LCD', monospace;
}

.lcddisplay {
  width: 280px;
  height: 94px;
  float: left;
  margin: 5px;
  background: #000000;
  border: 2px solid #333;
  -webkit-border-radius: 2px;
  -moz-border-radius: 2px;
  border-radius: 2px;
  line-height: normal;
}

.lcddisplay .lcd-text{
  float:left;
  margin: 5px 16px;
}

.lcd-line{
  float:left;
  clear:left;
  font-size: 16px;
  font-weight: normal;
  font-style: normal;
  color: #FFFF00;
  white-space: pre;
}

</style>