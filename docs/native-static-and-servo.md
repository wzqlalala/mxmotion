# Native static equilibrium, preload and effort servos

The native Motion workflow now supports three scenario analyses and two physical
effort actuator modes. They use the same imported CAD, motion JSON, constraints,
loads, flexible mesh and result channels as the existing native dynamics path.

## Scenario analyses

`scenarios.analysisType` accepts:

| Value | Behavior |
| --- | --- |
| `Dynamic` | Existing dynamics from the assembled initial state; the default for old JSON. |
| `Static` | Solve equilibrium under the actual loads evaluated at `t = 0`, with zero velocity. Return one converged frame at `t = 0`, with `StaticEquilibrium` termination. |
| `PreloadedDynamics` | Solve that equilibrium first, then start dynamics and any enabled PID controllers from the equilibrated state. |

The preload operation does **not** redefine spring rest lengths, reset the FEM
reference nodes, zero modal coordinates, or turn the stressed shape into a new
stress-free mesh. The converged state and its elastic forces are carried into
the transient solve.

Static controls are `staticMaxIterations` (default 100, maximum 1000),
`staticTolerance` (default `1e-8`), `staticConstraintTolerance` (default `1e-7`)
and `staticStepDamping` (default 1, in `(0, 1]`). The first tolerance applies to
the infinity norm of the actual generalized force balance `F + Cqᵀ λ`, including
constraint reactions, divided by the fixed initial generalized load norm (with
a floor of one engine unit). Keeping that reference fixed distinguishes real
nonlinear convergence from a shrinking denominator and gives unloaded FEM nodes
the same numerical load scale as the loaded system. This is a dimensionless
convergence measure, not a force measured in newtons.
The constraint tolerance applies to the native residual rows, which mix linear
and angular constraint quantities. The separate native joint acceptance check
continues to report closure and direction errors in mm and degrees.

The adapter runs checked Newton iterations with backtracking. Small mechanisms
use a central difference of the real engine forces to obtain a tangent, including
legacy force elements that do not supply a stiffness block. Larger FEM systems
use the engine's sparse stiffness and constraint matrices with sparse QR.
Both paths scale the KKT rows and columns before factorization, so large FEM
stiffness coefficients do not cause real interface constraint rows to disappear
under a numerical rank threshold. Contact candidates are refreshed at trial
geometries; support reactions are recomputed for the current small rigid-contact
manifold before checking equilibrium.
Zero correction, a solver return value, or exhaustion of the iteration budget
does not imply convergence. Singular inconsistent load directions, nonfinite
states, failure to reduce the residual, and iteration exhaustion return a failed
solve without replacing the previous result. Cancellation also leaves the
previous result intact.

A stationary equilibrium cannot satisfy a nonzero prescribed initial speed;
such a scenario reports the offending drive. Loads are evaluated at the
initial time, not averaged over the dynamic duration. An enabled discrete PID
requires `Dynamic` or `PreloadedDynamics`; in the latter it starts after
equilibrium. Static analysis does not execute a time-domain controller to invent
a holding force. Contact and nonsmooth mechanisms still require an appropriate
initial configuration and a convergent local equilibrium iteration.

## Actual force and torque motors

`drives.controlMode = Force` creates `MotionLinkMotorLinearForce` for a
translation. `Torque` creates `MotionLinkMotorRotationTorque` for a rotation.
Both attach to the selected joint's actual marker frames. Their guide constraints
are disabled because the existing passive joint supplies the guidance; an effort
motor contributes **zero** prescribed-motion constraint rows. A runtime check
enforces that distinction.

The existing expression/table editor defines the effort command in N or N·mm.
The motor applies the equal and opposite physical load to its two endpoints.
Force therefore produces acceleration according to mass and torque according to
inertia; it does not impose the requested position or speed.

| Drive field | Meaning | Units |
| --- | --- | --- |
| `effortLimit` | Symmetric force/torque magnitude cap; zero means no cap. | N or N·mm |
| `powerLimit` | Cap on absolute instantaneous mechanical power; zero disables. Applies during motoring and braking. | W |
| `noLoadSpeed` | Linear motoring torque/force versus speed envelope; requires a positive `effortLimit`. Zero disables. | mm/s or °/s |

The envelope is `effortLimit × max(0, 1 − abs(speed) / noLoadSpeed)` while the
command drives in the direction of motion. Braking remains available within the
effort and power bounds, including above the no-load speed. This is a physical
effort envelope, not an artificial clamp on the body's velocity.

The limiter evaluates the motor's current speed at force evaluation. Mechanical
power is `F[N] × v[mm/s] / 1000`, or
`T[N·mm] × omega[rad/s] / 1000`. The final engine boundary converts N and N·mm
to the native kg-mm-s force units. No conversion is applied to the public
command table, output limit, or controller gains.

## Effort PID and existing velocity PID

A PID controller may target a velocity, force, or torque drive. Effort PID uses
the actual position or velocity of the motor or a supported body/distance/angle
sensor. For the new effort modes, translational feedback uses mm (or mm/s) and
rotational feedback uses degrees (or degrees/s). The output is N or N·mm.

The effort control law is proportional plus integral feedback with derivative
on the measured signal. It does not add a velocity trajectory derivative to a
force command. A positive `controllers.outputLimit` applies in N or N·mm, in
addition to the motor's physical effort, power and motoring speed bounds.
Conditional integration prevents windup when either limit is active and the
error would drive the controller farther into saturation.

The controller samples the accepted state at `t = 0` and after each accepted
output step. Its command is held for the following interval. Force and reaction
feedback sensors remain unavailable to the PID loop because a consistent
within-step reaction sampling contract has not been implemented. They remain
available for result visualization.

Old velocity PID JSON retains its established units and behavior. In particular,
rotary direct-DOF feedback/targets use radians, angular sensor feedback/targets
use degrees, and the rotary velocity command/output limit uses rad/s. The GUI
and result channels identify those units rather than silently changing existing
gains or setpoints.

## Results

Effort drives expose `EffortCommand`, `EffortApplied`, `MechanicalPower`,
`EffortLimited`, `Coordinate`, `Rate` and `CoordinateAcceleration`, in addition
to their actuator and reaction vectors. The coordinate channels are measured
motion; they are not labelled `ImposedValue` or `ImposedSpeed`. Drive sensors
accept the corresponding actual-motion variables.

Actuator vectors retain the established J-end wrench convention so the existing
physical endpoint load-export conversion remains consistent. PID target,
feedback, error and output channels now carry their applicable units. Static
results include the iteration count and final residuals in the diagnostics.

## Numerical checks

`MotionOcc.StaticServo` exercises the complete CAD-to-native-solver path with:

- A 1 kg mass on a 10 N/mm spring under gravity: `kx = mg`, a single static
  output frame, and the expected support force.
- The same equilibrium continued into dynamics with unchanged position and
  velocity, checking preservation of preload without an initial impulse.
- Unbalanced free motion, iteration exhaustion and cancellation, checking that
  no false convergence replaces the previous result.
- A torsion spring with an applied torque: the expected `T/k = 10°` equilibrium.
- A constrained support balancing the 9.81 N weight through a real joint
  reaction, and a CAD box supported by compressed contact. The contact case
  explicitly requests a `1e-4` relative force tolerance for the collision
  geometry's precision and continues into dynamics without a preload impulse.
- Separate force and torque motors: the `F/m` and `T/I` acceleration, physical
  effort bounds, and J-end wrench sign.
- Power and no-load speed envelope behavior, plus position control through real
  force/torque motors with inertia, saturation, both feedback polarities,
  convergence and result units.

The existing velocity PID regression remains in `MotionOcc.NativeDynamics`.
