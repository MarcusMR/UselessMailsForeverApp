from flask import Flask, request, jsonify
import json

app = Flask(__name__)

@app.route('/addSub', methods=['POST'])
def post():
    if request.method == 'POST':
        data = request.form

        with open('subscribe.json', 'r') as json_file:
            data_dict = json.load(json_file)
        
        data_dict.append({key: data[key] for key in data.keys()})

        with open('subscribe.json', 'w') as json_file:
            json.dump(data_dict, json_file, indent=4)

        return "<h1>Successfully added you subscription</h1>"

if __name__ == '__main__':
    app.run(debug=True)
