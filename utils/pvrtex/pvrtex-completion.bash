#/usr/bin/env bash

#install pvrtex-completion.bash ~/.local/share/bash-completion/completions/pvrtex-completion.bash
#source pvrtex-completion.bash

_pvrtex_completions()
{
	local cur prev words cword
	_init_completion || return
	
	case $prev in
		--help|--version|--no-mip-shift|--max-color|--perfect-mip|--high-weight|--dither|--stride|--bilinear|--nearest|\
		-!(-*)[hvCSMHdsbn])
			return
			;;
		-i|--in)
			_filedir "@(png|jpg|bmp|tga|gif|psd|hdr|pic|pmn|pvr|dt)"
			return
			;;
		-P|--palette)
			_filedir "@(pal|png|jpg|bmp|tga|gif|psd|hdr|pic|pmn)"
			return
			;;
		-o|--out)
			_filedir "@(dt|tex|pvr)"
			return
			;;
		-p|--preview)
			_filedir "@(png|jpg|bmp|tga)"
			return
			;;
		-f|--format)
			COMPREPLY=($(compgen -W "rgb565 argb4444 argb1555 yuv pal4bpp pal8bpp bumpmap normal auto autoyuv" "$cur"))
			return
			;;
		-r|--resize)
			COMPREPLY=($(compgen -W "none near up down" "$cur"))
			return
			;;
		-R|--mip-resize)
			COMPREPLY=($(compgen -W "none x2 x4 up down opt" "$cur"))
			return
			;;
		-e|--edge)
			COMPREPLY=($(compgen -W "clamp wrap reflect zero" "$cur"))
			return
			;;
		-c|--compress)
			COMPREPLY=($(compgen -W "small" "$cur"))
			return
			;;
		--normal-style)
			COMPREPLY=($(compgen -W "texconv regular" "$cur"))
			return
			;;
		*)
			
			#This is the suggestion if not suggesting for one of the above. It suggests supported options.
			COMPREPLY=($(compgen -W "--in --out --preview --format --compress --mipmap --perfect-mip --max-color --no-mip-shift --high-weight --high-weight --dither --stride --resize --mip-resize --edge --bilinear --nearest --normal-style --flip-v" -- "$cur"))
			return
			;;
		
	esac
} && complete -F _pvrtex_completions pvrtex
