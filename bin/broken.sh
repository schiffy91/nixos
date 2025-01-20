


          if [ ! -e "$CURRENT_SNAPSHOT/$link" ]; then
            mkdir -p "$(dirname "$CURRENT_SNAPSHOT/$link")"
            cp -a "$link" "$CURRENT_SNAPSHOT/$link"
            echo "Successfully copied '$link' to '$CURRENT_SNAPSHOT/$link'"
          fi

      log_info "Copying '$PATHS_TO_KEEP' to '$CURRENT_SNAPSHOT'..."
      for path in $PATHS_TO_KEEP; do
        current_path="$ROOT/$path"
        tmp_path="$CURRENT_SNAPSHOT/$path"
        if [ -e "$current_path" ]; then
          mkdir -p "$(dirname "$tmp_path")"
          cp -a "$current_path" "$tmp_path"
          echo "Successfully copied '$current_path' to '$tmp_path'"
        else
          echo "Warning: '$current_path' does not exist and was not preserved."
        fi
      done