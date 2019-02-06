'use strict';

function enter(args, ctx) {
    window.AWSIotData.getThingShadow({
        thingName: 'devopstar-accl-01'
    }, (error, data) => {
        if (error) {
            console.error('Error getting state', error);

            return ctx.transitions.failure();
        }

        const payload = JSON.parse(data.payload);

        ctx.behaviorData.a_x = payload.state.reported.a_x;
        ctx.behaviorData.a_y = payload.state.reported.a_y;
        ctx.behaviorData.a_z = payload.state.reported.a_z;

        ctx.transitions.success();
    });
}

function exit(args, ctx) {

}

var parameters = [];
