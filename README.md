# Just Kill

It's a top-down survival zombie shooter game. You need to survive for as long as possible with your friends.
Tests has been done with 4 players but the game can theoretically handle more.

## Authors
* **Adrià Ferrer** - [Adria-F](https://github.com/Adria-F)
* **Josep Pi** - [joseppi](https://github.com/joseppi)

## Getting Started
- You can revive a fallen ally by staying close to them.
- W,A,S,D for moving your player.
- Left Mouse Button shoot.
- F1 to toggle between Network UI and GUI

## Features
### Adrià Ferrer
- **Entity Interpolation**
	- Fixed Bug: Then a player died the interpolation did not stop
- **Redundancy**
- Client Side Prediction 
	- Behaveour Implementation
- Path finding
	- Separation
- Revive player
	- Multiple player reviving time 
	- Fixed Bug: Players could not revive faster when there where more than one
- Player Names
	- Names of each player are displayed on the top of their Game Object.
	- Can't have two players with the same name.
- Client Laser
- Invincibility on spawn
- Release

### Josep Pi
- **Client Side Prediction**
- Delegate
- Zombie Spawner
	- Added Increasing Zombie Spawning Ratio
	- Zombie Spawner based on connected proxies
	- Server and spawner resets when no proxies connected on the server 
- Fix revive bug 
- Server clears GameObject list when there are no proxies connected.
- Update Colliders Module to the last version
- Map
- Collision of the map
- Improved ImGui debug GUI
	- You can see what packets are begin dropped
	- Log jumps to the last event automatically
- ReadMe

### Both
- **Server Replication**
- **Delivery Manager**
	- Fixed Bug: Made sure packet drops did not cause any sync problems. (Josep)
- **Server Reconciliation**

## Known Bugs
- Sometimes when you spawn and there are a lot of zombies on the player spawn zone, it can create some issues.