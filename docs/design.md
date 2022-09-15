# Our design

## Controller

Each controller essentially needs chunks to be loaded for its logic to be running, these chunks are known to the backend server and the backend will use that information to choose a worker

## Worker

Worker is a single machine, it syncs only the chunks which are required for itself alive and will tell the backend whenever he does not have anyone using a chunk anymore.

It takes the work from all its controllers and dispatches it to executers.

## Executer

Executers are essentially threads in a single machine, they actually run game logic and do work.
