'use strict';

function enter(args, ctx) {
    try {
        var boxForce = new sumerian.Vector3(
            ctx.behaviorData.a_x,
            ctx.behaviorData.a_y,
            ctx.behaviorData.a_z
        )
        ctx.entity.rigidBodyComponent.setAngularVelocity(boxForce);

        if (ctx.behaviorData.a_x != 0 || ctx.behaviorData.a_x != 0 || ctx.behaviorData.a_x != 0) {
            window.AWSIotData.updateThingShadow({
                    thingName: 'devopstar-accl-01',
                    payload: '{\"state": {\"reported\": {\"a_x\": 0,\"a_y\": 0,\"a_z\": 0}}}'
                },
                (error, data) => {
                    console.log(error)
                }
            );
        }
    } catch (error) {
        console.error('Error rotating box', error);

        return ctx.transitions.failure();
    }

    ctx.transitions.success();
}

function exit(args, ctx) {}

var parameters = [];
