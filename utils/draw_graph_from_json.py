import sys
import json
import html
import graphviz

def shorten(s, length=8):
    if type(s) == list:
        return "[...]"
    s = str(s)
    if len(s) > length:
        return s[:length-3]
    else:
        return s

def edge_label(name, net):
    bgcolors = {
        "default": "white",
        "OANALOG": "turquoise",
        "EANALOG": "orange",
    }
    try:
        bgcolor = bgcolors[net["type"]]
    except:
        bgcolor = bgcolors["default"]

    short_name = name
    if short_name.startswith('ROOT/'):
        short_name = short_name[5:]

    attributes = [f'<table style="rounded" border="0" cellborder="1" cellspacing="10" bgcolor="{bgcolor}" cellpadding="4" align="center">']
    attributes.append(f'<tr><td bgcolor="grey91" port="head"><b>{html.escape(short_name)}</b></td></tr>')

    attributes.append(f'<tr><td bgcolor="{bgcolor}" port="middle" border="0"><table border="0" cellborder="1" cellspacing="0">')
    # attributes.append(f'<tr><td bgcolor="{bgcolor}"><table border="0" cellborder="1" cellspacing="0" cellpadding="0">')
    attributes.append(f'<tr><td bgcolor="grey91">Type</td><td bgcolor="white">{net["type"]}</td></tr>')
    attributes.append(f'<tr><td bgcolor="grey91">Bidirectional</td><td bgcolor="white">{"true" if net["bidirectional"] else "false"}</td></tr>')
    attributes.append(f'<tr><td bgcolor="grey91">Size</td><td bgcolor="white">{net["size"]}</td></tr>')
    attributes.append(f'<tr><td bgcolor="grey91">Readers</td><td bgcolor="white">{net["readers"]}</td></tr>')
    attributes.append(f'<tr><td bgcolor="grey91">Writers</td><td bgcolor="white">{net["writers"]}</td></tr>')
    attributes.append(f'</table></td></tr>')

    attributes.append('</table>')
    return f'"{name}" [label=<{"".join(attributes)}> shape=none fillcolor="{bgcolor}" margin="0.05"]'



def node_label(element):
    attributes = ['<table border="0" cellborder="1" cellspacing="0" cellpadding="4" align="center">']

    short_name = element["name"]
    if short_name.startswith('ROOT/'):
        short_name = short_name[5:]
    # Header
    attributes.append(f'<tr><td bgcolor="grey91"><b>{html.escape(short_name)}</b></td></tr>')

    # Nets
    attributes.append(f'<tr><td bgcolor="darkslategray2"><table border="0" cellborder="1" cellspacing="8">')
    if 'nets' in element and element['nets']:
        nets = ''.join(f'<tr><td bgcolor="white"  cellpadding="4" port="{html.escape(str(i))}">{str(i)}</td></tr>' for i,net in enumerate(element['nets']))
        attributes.append(f'{nets}')
    else:
        attributes.append('<tr><td border="0" cellpadding="0">&empty;</td></tr>')
    attributes.append(f'</table></td></tr>')

    # Args
    attributes.append(f'<tr><td bgcolor="palegreen"><table border="0" cellborder="1" cellpadding="2" cellspacing="0">')
    if 'args' in element and element['args']:
        args = ''.join(f'<tr><td bgcolor="white"  cellpadding="4">{html.escape(shorten(v["value"]))}</td></tr>' for v in element['args'])
        attributes.append(f'{args}')
    else:
        attributes.append('<tr><td border="0" cellpadding="0">&empty;</td></tr>')
    attributes.append(f'</table></td></tr>')

    # Kwargs
    attributes.append(f'<tr><td bgcolor="wheat"><table border="0" cellborder="1" cellpadding="2" cellspacing="0">')
    if 'kwargs' in element and element['kwargs']:
        kwargs = ''.join(f'<tr><td bgcolor="grey91">{html.escape(k)}</td><td bgcolor="white">{html.escape(shorten(v["value"]))}</td></tr>' for k, v in element['kwargs'].items())
        attributes.append(f'{kwargs}')
    else:
        attributes.append('<tr><td border="0" cellpadding="0">&empty;</td></tr>')
    attributes.append(f'</table></td></tr>')

    attributes.append('</table>')
    return f'"{element["name"]}" [label=<{"".join(attributes)}> shape=none margin="0"]'

def edge(element):
    if 'nets' in element:
        for i,net in enumerate(element['nets']):
            # yield f'"{element["name"]}":"{str(i)}" -- "{net}":"head"'
            yield f'"{element["name"]}":"{str(i)}" -> "{net}":"middle"[ arrowhead = none ]'

def generate_graph(description):
    result = []
    result.append('digraph G {')
    # result.append('  fontname="Helvetica,Arial,sans-serif"')
    # result.append('  node [fontname="Helvetica,Arial,sans-serif"]')
    # result.append('  edge [fontname="Helvetica,Arial,sans-serif"]')
    result.append('  fontname="Cascadia,Courrier,mono"')
    result.append('  node [fontname="Cascadia,Courrier,mono"]')
    result.append('  edge [fontname="Cascadia,Courrier,mono"]')
    result.append('  layout="sfdp"')
    result.append('  overlap=false')
    result.append('  splines=curved')

    for name, net in description["nets"].items():
        result.append('  ' + edge_label(name, net))

    for element in description["elements"]:
        result.append('  ' + node_label(element))
        result.extend('  ' + e for e in edge(element))

    result.append('}')
    return '\n'.join(result)

def draw_graph(json_file, out_file):
    with open(json_file) as f:
        description = json.load(f)

    dot_src = generate_graph(description)
    # print(dot_src)
    graph = graphviz.Source(dot_src)
    graph.render(outfile=out_file)
    print(f'Results written to {out_file}')

def main():
    try:
        json_file = sys.argv[1]
    except IndexError:
        print('First and only argument should be a JSON description of the circuit')
    try:
        out_file = sys.argv[2]
    except IndexError:
        out_file = json_file + ".png"

    with open(json_file) as f:
        description = json.load(f)

    dot_src = generate_graph(description)
    # print(dot_src)
    graph = graphviz.Source(dot_src)
    graph.render(outfile=out_file)
    print(f'Results written to {out_file}')

if __name__ == '__main__':
    main()
