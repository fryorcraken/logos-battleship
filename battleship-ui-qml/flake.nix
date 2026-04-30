{
  description = "Battleship QML UI plugin for Logos — QML frontend for battleship module";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/tutorial-v1";
    battleship.url = "path:../battleship";

    # Pin battleship's logos-module-builder to ours to avoid transitive
    # version conflicts. Same pattern as tictactoe.
    battleship.inputs.logos-module-builder.follows = "logos-module-builder";

    # Pin battleship's delivery_module's module-builder to ours too.
    battleship.inputs.delivery_module.inputs.logos-module-builder.follows = "logos-module-builder";
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
