" Vim keyboard shortcut for make&run.
nnoremap <leader>m :wa<bar>!xterm -e 'make&&bin/main;read' &<cr><cr>
