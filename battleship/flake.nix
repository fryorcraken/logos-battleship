{
  description = "Battleship Logos module - wraps libbattleship C library";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/tutorial-v1";
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;

      # Workaround: module-builder's `vendor_path` external-library handler
      # only copies `lib*` files from the vendor dir — it does NOT run
      # `build_command`. Without this, the plugin links with undefined
      # `bs_*` symbols and crashes at first call inside basecamp.
      # See: https://github.com/logos-co/logos-module-builder/issues/83
      preConfigure = ''
        if [ -f lib/libbattleship.c ] && [ ! -f lib/libbattleship.so ]; then
          echo "Compiling vendored libbattleship.so..."
          (cd lib && gcc -shared -fPIC -o libbattleship.so libbattleship.c)
        fi
      '';
    };
}
