import { BrewPiSensor } from '@/mixins/BrewPiSensor.js';

describe('BrewPiSensor', () => {
    it('can convert from BrewPi device spec', () => {
        const sensor = new BrewPiSensor();
        const device_spec = {
            c: 1,
            b: 2,
            f: 3,
            h: 4,
            p: 5,
            x: true,
            d: true,
            i: 6,
            a: "address",
            r: "alias",
            n: "child_id",
            j: 7,
        };
        sensor.convertFromBrewPi(device_spec);

        // add checks for each property
        expect(sensor.chamber).toBe(1);
        // ...
    });

    it('can convert to BrewPi device spec', () => {
        const sensor = new BrewPiSensor();
        sensor.chamber = 1;
        sensor.beer = 1;
        sensor.device_function_int = 2;
        sensor.hardware_int = 3;
        sensor.pin = 4;
        sensor.invert = true;
        sensor.deactivated = true;
        sensor.address = "TestAddress";
        sensor.device_alias = "TestAlias";
        sensor.child_id = "TestChildID";
        sensor.calibrate_adjust = 5;
        sensor.index = 6;

        const device_spec = sensor.convertToBrewPi();

        // add checks for each property
        expect(device_spec.c).toBe(sensor.chamber);
        expect(device_spec.b).toBe(sensor.beer);
        expect(device_spec.f).toBe(sensor.device_function_int);
        expect(device_spec.h).toBe(sensor.hardware_int);
        expect(device_spec.p).toBe(sensor.pin);
        expect(device_spec.x).toBe(sensor.invert);
        expect(device_spec.d).toBe(sensor.deactivated);
        expect(device_spec.a).toBe(sensor.address);
        expect(device_spec.n).toBe(sensor.child_id);
        expect(device_spec.i).toBe(sensor.index);
    });

    it('includes calibrate_adjust in device_spec if it is non-zero and hardware_int is 2, 5, or 6', () => {
        const sensor = new BrewPiSensor();
        sensor.hardware_int = 2;
        sensor.calibrate_adjust = 5;

        const device_spec = sensor.convertToBrewPi();

        expect(device_spec.j).toBe(sensor.calibrate_adjust);
    });

    it('does not include calibrate_adjust in device_spec if it is zero', () => {
        const sensor = new BrewPiSensor();
        sensor.hardware_int = 2;
        sensor.calibrate_adjust = 0;

        const device_spec = sensor.convertToBrewPi();

        expect(device_spec.j).toBeUndefined();
    });

    it('does not include calibrate_adjust in device_spec if hardware_int is not 2, 5, or 6', () => {
        const sensor = new BrewPiSensor();
        sensor.hardware_int = 1;
        sensor.calibrate_adjust = 5;

        const device_spec = sensor.convertToBrewPi();

        expect(device_spec.j).toBeUndefined();
    });


    it('returns the correct device function', () => {
        const sensor = new BrewPiSensor();
        sensor.device_function_int = 1;

        const device_function = sensor.device_function;

        expect(device_function).toBe('chamber_door');
        // TODO - add similar checks for other values
    });

    it('returns the correct device hardware', () => {
        const sensor = new BrewPiSensor();
        sensor.hardware_int = 1;

        const device_hardware = sensor.device_hardware;

        expect(device_hardware).toBe('pin');
        // TODO - add similar checks for other values
    });

    it('returns the correct valid functions for hardware type', () => {
        const sensor = new BrewPiSensor();
        sensor.hardware_int = 1;  // for pin

        const valid_functions = sensor.valid_functions();

        // TODO - add checks for each function
        expect(valid_functions).toContainEqual({id: 0, function_name: 'none'});
        expect(valid_functions).toContainEqual({id: 1, function_name: 'chamber_door'});
        // ...
    });
});
