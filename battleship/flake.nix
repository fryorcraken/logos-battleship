{
  description = "Battleship Logos module - wraps libbattleship C library";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/tutorial-v1";

    # Pin: head of `tutorial-v1-compat` on logos-delivery-module.
    # Must stay pinned so the wire format matches basecamp's bundled delivery_module.
    # See: https://github.com/logos-co/logos-delivery-module/pull/23
    delivery_module.url = "github:logos-co/logos-delivery-module/1fde1566291fe062b98255003b9166b0261c6081";

    # Force delivery_module's transitive logos-module-builder to follow ours.
    # Without this, delivery_module drags in its own master-branch module-builder
    # which breaks wire-format compatibility with basecamp's bundled delivery_module.
    # See: https://github.com/logos-co/logos-module-builder/issues/83
    delivery_module.inputs.logos-module-builder.follows = "logos-module-builder";
  };

  outputs = inputs@{ logos-module-builder, delivery_module, ... }:
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
          (cd lib && gcc -shared -fPIC -o libbattleship.so libbattleship.c sha256.c)
        fi
      '';
    };
}
