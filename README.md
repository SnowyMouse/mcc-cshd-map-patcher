# MCC CEA Client-Side Hit Detection Map Patcher

This program allows you to patch an MCC map so that all maps use client-side hit
detection instead of just the pistol and sniper rifle.

The map must have a `weapons\pistol\pistol.weapon` somewhere for this to work.

## Compiling

To compile, you will need GNU Make and a C99 compiler.

Note that this will only work on little endian machines.

## Usage

Patching:

`$ mcc-cshd-map-patcher maps/bloodgulch.map patch`

Undoing:

`$ mcc-cshd-map-patcher maps/bloodgulch.map undo`

## What is client side hit detection?

On an online multiplayer FPS game like Halo, clients cannot simulate the same
exact game instance. This is simply the nature of the laws of physics: network
connections do not provide a low enough latency for this to be possible while
also allowing the game to be enjoyable with a low input delay. To avoid an
increased input latency, games will allow for some inherent desyncing due to
ping. However, the game needs to account for this.

In this case, we're focusing on how projectiles are synced. MCC: CEA's netcode
has three ways of handling projectiles:
* Sync the projectile from the client (client side hit detection)
* Sync the projectile from the host
* Do not sync the projectile

Precision weapons (the sniper rifle and pistol) sync from the client. This means
that the client's projectile (and simulation of the game) is used for damage
calculation. Therefore, any projectile that hits on the client will successfully
register as a hit, barring significant packet loss. If the projectile hits on
the host but not the client, the damage is ignored.

It's also worth noting that the effect of weapons spinning when shot will only
work if you are the host or you have client side hit detection.

Slow moving projectiles like the rockets or thrown grenades will sync from the
host. Essentially, what this means is that, when the weapon is fired, the
projectile will not appear on the client until the projectile actually appears
on the host. This is the inverse of the first one: if the projectile hits on the
client but not the host, the damage is ignored.

All other weapons use the last method. This means that neither the projectile
that appears on the host nor the one that appears on the client will match,
but the projectile on the host will still be used for damage calculation.

Unfortunately, this means that using most weapons require leading based on ping
if you are not the host, and thus the weapons are inherently unreliable, similar
to the original, broken Gearbox port's netcode (except the pistol and sniper
rifle are immune to this).

In fact, most modern games provide some sort of ping compensation, even if not
client side hit detection. Some games, for example, will use a previous state of
the game to do a server side hit detection. This is what the SAPP mod for Halo
PC uses.

However, client side hit detection will provide the best experience, as it will
prevent "blood shots" (shots that appear to hit but do not register).

At the time of writing this, 343 Industries have not addressed this issue.

## How does this program fix this issue?

It doesn't. However, it does work around the issue by changing the tag paths of
all weapons (except the sniper rifle) to `weapons\pistol\pistol` while recording
the original tag path pointer so it can be undone (if desired). This tricks the
game into applying CSHD to all weapons, though this will not fix melees.

The real fix is to not make this check based on weapon but have it available to
all weapons. Until this is addressed (assuming it ever will be), the only way to
accomplish this is to do the aforementioned trick.
